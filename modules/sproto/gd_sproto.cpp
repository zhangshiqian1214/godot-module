#include <stdlib.h>
#include <memory.h>
#include "gd_sproto.h"

extern "C" {
#include "thirdparty/sproto/sproto.h"
};

#define ENCODE_MAXSIZE 0x1000000
#define ENCODE_DEEPLEVEL 64

Sproto::Sproto()
{
	m_cacheSprotoIndex = 0;
	m_cacheSprotoTypeIndex = 0;
}

Sproto::~Sproto()
{
	
}

//err: < 0
int Sproto::new_proto(const ByteArray& buffer)
{
	struct sproto* sp;
	sp = sproto_create(buffer.read().ptr(), buffer.size());
	if (sp){
		int index = set_sproto_ptr(sp);
		return index;
	}
	ERR_EXPLAIN("create new sproto error");
	return -1;
}

int Sproto::delete_proto(int sproto_ptr)
{
	struct sproto* sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL){
		ERR_EXPLAIN("delete sproto error, sproto_ptr:" + String::num(sproto_ptr));
		return -1;
	}
	sproto_release(sp);
	del_sproto_ptr(sproto_ptr);
	return 0;
}

int Sproto::save_proto(int sproto_ptr, int saveindex)
{
	struct sproto* sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL)
	{
		ERR_EXPLAIN("save_proto sproto NULL, sproto_ptr:" + String::num(sproto_ptr));
		return -1;
	}
	if (saveindex < 0 || saveindex >= MAX_GLOBALSPROTO)
	{
		ERR_EXPLAIN("save_proto saveindex err, saveindex:" + String::num(saveindex));
		return -1;
	}
	g_sproto[saveindex] = sp;
	return 0;
}

int Sproto::load_proto(int saveindex)
{
	struct sproto* sp;
	if (saveindex < 0 || saveindex >= MAX_GLOBALSPROTO)
	{
		ERR_EXPLAIN("load_proto saveindex err, saveindex:" + String::num(saveindex));
		return -1;
	}
	sp = g_sproto[saveindex];
	if (sp == NULL)
	{
		ERR_EXPLAIN("load_proto sproto NULL, saveindex:" + String::num(saveindex));
		return -1;
	}
	int index = set_sproto_ptr(sp);
	return index;
}

int Sproto::dump_proto(int sproto_ptr)
{
	struct sproto* sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL)
	{
		ERR_EXPLAIN("dump_proto sproto NULL, sproto_ptr:" + String::num(sproto_ptr));
		return -1;
	}
	sproto_dump(sp);
	return 0;
}

int Sproto::query_type(int sproto_ptr, const String& type_name)
{
	struct sproto_type* st;
	struct sproto* sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL){
		ERR_EXPLAIN("query_type query sproto_type err sproto_ptr:" + String::num(sproto_ptr));
		return -1;
	}

	st = sproto_type(sp, type_name.utf8().get_data());
	if (st == NULL){
		ERR_EXPLAIN("query_type sproto_type err sproto_ptr:" + String::num(sproto_ptr) + " type_name:" + type_name);
		return -1;
	}
	int index = set_sproto_type_ptr(st);

	return index;
}

Array Sproto::protocol(int sproto_ptr, const Variant& pname)
{
	Array ret;
	int t;
	int tag;
	struct sproto_type * request;
	struct sproto_type * response;
	struct sproto * sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL){
		ERR_EXPLAIN("protocol sproto NULL, sproto_ptr:" + String::num(sproto_ptr));
		ret.push_back(-1);
		return ret;
	}
	if (pname.is_num()){
		const char * name;
		tag = pname.operator int();
		name = sproto_protoname(sp, tag);
		if (name == NULL)
			ret.push_back(-1);
			return ret;
		ret.push_back(pname);
	}
	else{
		const char * name = pname.operator String().utf8().get_data();
		if (name == NULL){
			ret.push_back(-1);
			return ret;
		}
		tag = sproto_prototag(sp, name);
		if (tag < 0){
			ret.push_back(-1);
			return ret;
		}
		ret.push_back(tag);
	}
	request = sproto_protoquery(sp, tag, SPROTO_REQUEST);
	if (request == NULL){
		ret.push_back(Variant());
	}
	else{
		int request_index = set_sproto_type_ptr(request);
		ret.push_back(request_index);
	}
	response = sproto_protoquery(sp, tag, SPROTO_RESPONSE);
	if (response == NULL){
		ret.push_back(Variant());
	}
	else
	{
		int response_index = set_sproto_type_ptr(response);
		ret.push_back(response_index);
	}
	return ret;
}

static int 
encode_default(const struct sproto_arg *args)
{
	Dictionary& dict = *(Dictionary *) args->ud;
	Variant value;
	if (args->index > 0) {
		if (args->mainindex > 0)
			value = Dictionary(true);
		else
			value = Array(true);

		dict[args->tagname] = value;
		return SPROTO_CB_NOARRAY;
	} else {
		switch(args->type) {
		case SPROTO_TINTEGER:
			value = 0;
			break;
		case SPROTO_TBOOLEAN:
			value = false;
			break;
		case SPROTO_TSTRING:
			value = "";
			break;
		case SPROTO_TSTRUCT:
			Dictionary sub(true);
			sub["__type"] = sproto_name(args->subtype);
			value = sub;

			char dummy[64];
			sproto_encode(args->subtype, dummy, sizeof(dummy), encode_default, &sub);
			break;
		}
		dict[args->tagname] = value;
		return SPROTO_CB_NIL;
	}
}

Dictionary Sproto::get_default(int sproto_type_ptr)
{
	int ret;
	char dummy[64];
	struct sproto_type* st = get_sproto_type_ptr(sproto_type_ptr);
	if (st == NULL)
	{
		ERR_EXPLAIN("protocol sproto_type NULL, sproto_ptr:" + String::num(sproto_type_ptr));
		return Dictionary(true);
	}
	Dictionary dict(true);
	dict["__type"] = sproto_name(st);
	ret = sproto_encode(st, dummy, sizeof(dummy), encode_default, &dict);
	if (ret < 0){
		int sz = sizeof(dummy) * 2;
		Vector<char> vstr;
		vstr.resize(sz);
		char * tmp = vstr.ptr();
		for (;;) {
			ret = sproto_encode(st, tmp, sz, encode_default, &dict);
			if (ret >= 0)
				break;
			sz *= 2;
			vstr.resize(sz);
			tmp = vstr.ptr();
		}
	}

	return dict;
}

struct encode_ud {
	struct sproto_type *st;
	Variant value;
	const char *source_tag;
	Variant source;
	int deep;
};

static int
encode_callback(const struct sproto_arg * args)
{
	struct encode_ud * self = (struct encode_ud*) args->ud;
	Variant& value = self->value;
	Variant source = self->source;
	if (self->deep >= ENCODE_DEEPLEVEL){
		ERR_EXPLAIN("The table is too deep");
		return SPROTO_CB_ERROR;
	}

	if (args->index > 0){
		if (args->tagname != self->source_tag){
			// a new array
			self->source_tag = args->tagname;
			bool r_valid;
			source = value.get(args->tagname, &r_valid);
			//ERR_FAIL_COND_V(!r_valid, 0);
			if (source.get_type() == Variant::NIL)
				return SPROTO_CB_NOARRAY;
				//return SPROTO_CB_NIL;
			if (source.get_type() != Variant::DICTIONARY && source.get_type() != Variant::ARRAY) {
				ERR_FAIL_V(SPROTO_CB_NOARRAY);
			}
			self->source = source;
		}
		int index = args->index - 1;
		if (args->mainindex >= 0){
			// todo: check the key is equal to mainindex value
			if(source.get_type() == Variant::DICTIONARY) {
				Dictionary dict = source;
				if(index >= dict.size())
					return 0;
				const Variant *K=NULL;
				while((K=dict.next(K))) {
					if(index-- == 0) {
						source = dict[*K];
						break;
					}
				}
			}
			else if (source.get_type() == Variant::ARRAY){
				Array array = source;
				if(index >= array.size())
					return SPROTO_CB_NIL;
				source = array[index];
			}
		} else {
			if(source.get_type() == Variant::DICTIONARY) {
				Dictionary dict = source;
				if(!dict.has(index))
					return SPROTO_CB_NIL;
				source = dict[index];
			} 
			else if(source.get_type() == Variant::ARRAY) {
				Array array = source;
				if(index >= array.size())
					return SPROTO_CB_NIL;
				source = array[index];
			}
		}
	} else {
		if(value.get_type() != Variant::DICTIONARY && value.get_type() != Variant::ARRAY) {
			ERR_EXPLAIN(String(args->tagname)
				+ "("
				+ String::num(args->tagid)
				+ ") should be a dict/array (Is a "
				+ value.get_type_name(value.get_type())
				+ ")"
			);
			ERR_FAIL_V(SPROTO_CB_ERROR);
		}
		if(value.get_type() == Variant::DICTIONARY) {
			Dictionary dict = value;
			if(!dict.has(args->tagname))
				return SPROTO_CB_NIL;
			source = dict[args->tagname];
		}
		else if(value.get_type() == Variant::ARRAY) {
			Array array = value;
			int idx = atoi(args->tagname);
			if(idx >= array.size())
				return SPROTO_CB_NIL;
			source = array[idx];
		}
	}

	if(source.get_type() == Variant::NIL)
		return 0;

	switch (args->type) {
	case SPROTO_TINTEGER: {
		if (source.get_type() != Variant::INT && source.get_type() != Variant::REAL) {
			ERR_EXPLAIN(String(args->tagname)
				+ "("
				+ String::num(args->tagid)
				+ ") should be a int/real (Is a "
				+ source.get_type_name(source.get_type())
				+ ")"
			);
			ERR_FAIL_V(SPROTO_CB_ERROR);
		}
		long v = (double) source;
		// notice: godot only support 32bit integer
		long vh = v >> 31;
		if (vh == 0 || vh == -1) {
			if(args->length < 4)
				return -1;
			*(unsigned int *)args->value = (unsigned int)v;
			return 4;
		}
		else {
			*(unsigned long *)args->value = (unsigned long)v;
			if(args->length < 8)
				return -1;
			return 8;
		}
	}
	case SPROTO_TBOOLEAN: {
		if(source.get_type() != Variant::BOOL) {
			ERR_EXPLAIN(String(args->tagname)
				+ "("
				+ String::num(args->tagid)
				+ ") should be a bool (Is a "
				+ source.get_type_name(source.get_type())
				+ ")"
			);
			ERR_FAIL_V(SPROTO_CB_ERROR);
		}
		bool v = source;
		*(int *)args->value = v;
		return 4;
	}
	case SPROTO_TSTRING: {
		if(source.get_type() != Variant::STRING) {
			ERR_EXPLAIN(String(args->tagname)
				+ "("
				+ String::num(args->tagid)
				+ ") should be a string (Is a "
				+ source.get_type_name(source.get_type())
				+ ")"
			);
			ERR_FAIL_V(SPROTO_CB_ERROR);
		}
		String v = source;
		CharString utf8 = v.utf8();
		size_t sz = utf8.length();
		if(sz > args->length)
			return -1;
		memcpy(args->value, utf8.get_data(), sz);
		//printf("%s -> %d bytes\n", args->tagname, sz + 1);
		return sz;	// The length of empty string is 1.
	}
	case SPROTO_TSTRUCT: {
		struct encode_ud sub;
		sub.value = source;
		int r;
		if(source.get_type() != Variant::DICTIONARY && source.get_type() != Variant::ARRAY) {
			ERR_EXPLAIN(String(args->tagname)
				+ "("
				+ String::num(args->tagid)
				+ ") should be a dict/array (Is a "
				+ source.get_type_name(source.get_type())
				+ ")"
			);
			ERR_FAIL_V(SPROTO_CB_ERROR);
		}
		sub.st = args->subtype;
		sub.deep = self->deep + 1;
		r = sproto_encode(args->subtype, args->value, args->length, encode_callback, &sub);
		//if (r < 0)
			//ERR_FAIL_V(SPROTO_CB_ERROR);
		return r;
	}
	default:
		ERR_EXPLAIN("Invalid field type: " + String::num(args->type));
		ERR_FAIL_V(SPROTO_CB_ERROR);
	}
	return SPROTO_CB_ERROR;
}

ByteArray Sproto::encode(int sproto_type_ptr, const Dictionary& data)
{
	ByteArray result;
	struct sproto_type* st = get_sproto_type_ptr(sproto_type_ptr);
	if (st == NULL)
	{
		ERR_EXPLAIN("encode sproto_type NULL, sproto_ptr:" + String::num(sproto_type_ptr));
		ERR_FAIL_COND_V(st == NULL, ByteArray());
		//return ByteArray();
	}
	result.resize(1024);
	ByteArray::Write w = result.write();
	struct encode_ud self;
	self.value = data;
	self.st = st;
	for (;;){
		self.deep = 0;
		int r = sproto_encode(self.st, w.ptr(), result.size(), encode_callback, &self);
		w = ByteArray::Write();
		if (r < 0){
			result.resize(result.size() * 2);
			ByteArray::Write w = result.write();
		} else {
			result.resize(r);
			break;
		}
	}

	return result;
}

struct decode_ud {
	const char * array_tag;
	Variant result;
	Variant array;
	Variant key;
	int deep;
	int mainindex_tag;
};

static int decode_callback(const struct sproto_arg *args){
	struct decode_ud * self = (struct decode_ud*) args->ud;
	Variant& result = self->result;
	Variant& array = self->array;
	Variant value;
	if (args->index != 0){
		// It's array
		if (args->tagname != self->array_tag) {
			self->array_tag = args->tagname;
			Dictionary object = result.operator Dictionary();
			if(!object.has(args->tagname)) {
				if(args->mainindex >= 0) {
					array = Dictionary(true);
				} else {
					array = Array(true);
				}
				object[args->tagname] = array;
			} else {
				array = object[args->tagname];
			}
			if (args->index < 0) {
				// It's a empty array, return now.
				return 0;
			}
		}
	}
	switch(args->type) {
	case SPROTO_TINTEGER:{
		// notice: in lua 5.2, 52bit integer support (not 64)
		value = (*(int *)args->value);
		break;
	}
	case SPROTO_TBOOLEAN:{
		value = (bool) (*(unsigned long *)args->value);
		break;
	}
	case SPROTO_TSTRING:{
		String str;
		str.parse_utf8((const char *) args->value, args->length);
		value = str;
		break;
	}
	case SPROTO_TSTRUCT:{
		struct decode_ud sub;
		int r;
		value = Dictionary(true);

		sub.result = value;
		sub.deep = self->deep + 1;
		if (args->mainindex >= 0){
			// This struct will set into a map, so mark the main index tag.
			sub.mainindex_tag = args->mainindex;
			r = sproto_decode(args->subtype, args->value, args->length, decode_callback, &sub);
			if (r < 0)
				return SPROTO_CB_ERROR;
			if (r != args->length)
				return r;
			self->array.set(sub.key, value);
			return 0;
		} else {
			sub.mainindex_tag = -1;
			r = sproto_decode(args->subtype, args->value, args->length, decode_callback, &sub);
			if (r < 0)
				return SPROTO_CB_ERROR;
			if (r != args->length)
				return r;
			break;
		}
	}
	default:
		ERR_EXPLAIN("Invalid type:" + String::num(args->type) + " tag_name:" + args->tagname);
		break;
	}
	
	/*if (value.get_type() == Variant::NIL)
	{
		return SPROTO_CB_NOARRAY;
	}*/

	if (args->index > 0){
		Array object = self->array.operator Array();
		object.append(value);
	} else {
		if (self->mainindex_tag == args->tagid) {
			self->key = value;
		}
		Dictionary object = self->result.operator Dictionary();
		object[args->tagname] = value;
	}
	return 0;
}

Array Sproto::decode(int sproto_type_ptr, const ByteArray& buffer)
{
	Array result(true);
	Dictionary outdata(true);
	struct sproto_type* st = get_sproto_type_ptr(sproto_type_ptr);
	if (st == NULL)
	{
		ERR_EXPLAIN("decode sproto_type NULL, sproto_ptr:" + String::num(sproto_type_ptr));
		ERR_FAIL_COND_V(st == NULL, result);
		//return ByteArray();
	}
	struct decode_ud self;
	self.result = outdata;
	self.deep = 0;
	self.mainindex_tag = -1;
	ByteArray::Read r = buffer.read();

	int ret = sproto_decode(st, r.ptr(), buffer.size(), decode_callback, &self);
	if (ret < 0){
		ERR_EXPLAIN("decode fatal err, sproto_ptr:" + String::num(sproto_type_ptr) + " ret:" + String::num(ret));
		result.push_back(ret);
		return result;
	}
	result.push_back(ret);
	result.push_back(outdata);
	return result;
}

#define ENCODE_BUFFERSIZE 2050
#define ENCODE_MAXSIZE 0x1000000

static void
expand_buffer(ByteArray& buffer, int osz, int nsz)
{
	do {
		osz *= 2;
	} while (osz < nsz);
	if (osz > ENCODE_MAXSIZE) {
		ERR_EXPLAIN("expand_buffer err, object is too large (>" + String::num(ENCODE_MAXSIZE) + ")");
		return;
	}
	buffer.resize(osz);
	return;
}

ByteArray Sproto::pack(const ByteArray& buffer)
{
	ByteArray result;
	int bytes;
	int sz = buffer.size();
	ByteArray::Read r = buffer.read();
	const void * buf = r.ptr();
	int maxsz = (sz + 2047) / 2048 * 2 + sz;
	int osz = result.size();
	if (osz < maxsz){
		expand_buffer(result, osz, maxsz);
	}

	//ByteArray wbuffer;
	//ByteArray::Write w = wbuffer.write();
	ByteArray::Write w = result.write();
	void * output = w.ptr();

	bytes = sproto_pack(buf, sz, output, maxsz);
	if (bytes > maxsz) {
		ERR_EXPLAIN("pack err, return size = " + String::num(bytes));
		return ByteArray();
	}

	result.resize(bytes);
	// w = result.write();
	// memcpy(w.ptr(), r.ptr(), bytes);
	return result;
}

ByteArray Sproto::unpack(const ByteArray& buffer)
{
	ByteArray result;
	int sz = buffer.size();
	ByteArray::Read r = buffer.read();
	const void *buf = r.ptr();
	ByteArray::Write w = result.write();
	void *output = w.ptr();
	int osz = result.size();
	int ret = sproto_unpack(buf, sz, output, osz);
	if (ret < 0) {
		ERR_EXPLAIN("Invalid unpack stream");
		return ByteArray();
	}
	if (ret > osz){
		w = ByteArray::Write();
		expand_buffer(result, osz, ret);
		w = result.write();
		ret = sproto_unpack(buf, sz, output, ret);
		if (ret < 0) {
			ERR_EXPLAIN("Invalid unpack stream");
			return ByteArray();
		}
	}
	result.resize(ret);
	return result;
}

int Sproto::set_sproto_ptr(struct sproto* ptr)
{
	SprotoIndexMap::Element* element = m_cacheSprotoIndexMap.find(ptr);
	if (element != NULL)
	{
		return element->value();
	}

	m_cacheSprotoIndex++;
	m_cacheSprotoIndexMap[ptr] = m_cacheSprotoIndex;
	m_cacheIndexSprotoMap[m_cacheSprotoIndex] = ptr;
	return m_cacheSprotoIndex;
}

int Sproto::set_sproto_type_ptr(struct sproto_type* ptr)
{
	SprotoTypeIndexMap::Element* element = m_cacheSprotoTypeIndexMap.find(ptr);
	if (element != NULL)
	{
		return element->value();
	}
	m_cacheSprotoTypeIndex++;
	m_cacheSprotoTypeIndexMap[ptr] = m_cacheSprotoTypeIndex;
	m_cacheIndexSprotoTypeMap[m_cacheSprotoTypeIndex] = ptr;
	return m_cacheSprotoTypeIndex;
}

struct sproto* Sproto::get_sproto_ptr(int sproto_ptr)
{
	IndexSprotoMap::Element* element = m_cacheIndexSprotoMap.find(sproto_ptr);
	if (element != NULL)
	{
		return element->value();
	}
	return NULL;
}

struct sproto_type* Sproto::get_sproto_type_ptr(int sproto_type_ptr)
{
	IndexSprotoTypeMap::Element* element = m_cacheIndexSprotoTypeMap.find(sproto_type_ptr);
	if (element != NULL)
	{
		return element->value();
	}
	return NULL;
}

void Sproto::del_sproto_ptr(int sproto_ptr)
{
	struct sproto* sp = get_sproto_ptr(sproto_ptr);
	if (sp == NULL){
		return;
	}
	m_cacheIndexSprotoMap.erase(sproto_ptr);
	m_cacheSprotoIndexMap.erase(sp);
	
	m_cacheSprotoTypeIndexMap.clear();
	m_cacheIndexSprotoTypeMap.clear();
}

void Sproto::del_sproto_type_ptr(int sproto_type_ptr)
{
	struct sproto_type* st = get_sproto_type_ptr(sproto_type_ptr);
	if (st == NULL){
		return;
	}
	m_cacheIndexSprotoTypeMap.erase(sproto_type_ptr);
	m_cacheSprotoTypeIndexMap.erase(st);
}

void Sproto::clear_sprotos()
{
	for(SprotoIndexMap::Element *E=m_cacheSprotoIndexMap.front(); E; E=E->next()){

	}
	m_cacheSprotoIndexMap.clear();
	m_cacheIndexSprotoMap.clear();
	m_cacheSprotoTypeIndexMap.clear();
	m_cacheIndexSprotoTypeMap.clear();
}

void Sproto::_bind_methods()
{
	ObjectTypeDB::bind_method(_MD("new_proto", "buffer"),&Sproto::new_proto);
	ObjectTypeDB::bind_method(_MD("delete_proto", "sproto_ptr"),&Sproto::delete_proto);
	ObjectTypeDB::bind_method(_MD("save_proto", "sproto_ptr", "saveindex"),&Sproto::save_proto);
	ObjectTypeDB::bind_method(_MD("load_proto", "saveindex"),&Sproto::load_proto);
	ObjectTypeDB::bind_method(_MD("dump_proto", "sproto_ptr"),&Sproto::dump_proto);

	ObjectTypeDB::bind_method(_MD("query_type", "sproto_ptr", "type_name"),&Sproto::query_type);
	ObjectTypeDB::bind_method(_MD("protocol", "sproto_ptr", "tag"),&Sproto::protocol);
	ObjectTypeDB::bind_method(_MD("get_default", "sproto_ptr"),&Sproto::get_default);
	ObjectTypeDB::bind_method(_MD("decode", "sproto_type_ptr", "buffer"),&Sproto::decode);
	ObjectTypeDB::bind_method(_MD("encode", "sproto_type_ptr", "data"),&Sproto::encode);
	ObjectTypeDB::bind_method(_MD("pack", "buffer"),&Sproto::pack);
	ObjectTypeDB::bind_method(_MD("unpack", "buffer"),&Sproto::unpack);
}