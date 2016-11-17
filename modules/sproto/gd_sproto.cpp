#include "gd_sproto.h"

extern "C" {
#include "thirdparty/sproto/sproto.h"
};

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

Array Sproto::decode(int sproto_type_ptr, const ByteArray& buffer)
{
	Array ret;
	return ret;
}

ByteArray Sproto::encode(int sproto_type_ptr, const Dictionary& data)
{
	ByteArray ret;
	return ret;
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