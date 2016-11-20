/*************************************************************************/
/*  gd_sproto.h                                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef GD_SPROTO_H
#define GD_SPROTO_H

extern "C"{
	struct sproto;
	struct sproto_type;
};

#include "core/map.h"
#include "core/resource.h"

//最多可以保存的sproto
#define MAX_GLOBALSPROTO 16

class Sproto : public Reference {
public:
	
	typedef Map<int, struct sproto *> IndexSprotoMap;
	typedef Map<struct sproto *, int> SprotoIndexMap;
	typedef Map<int, struct sproto_type *> IndexSprotoTypeMap;
	typedef Map<struct sproto_type *, int> SprotoTypeIndexMap;

	Sproto();
	~Sproto();
	//返回sproto_ptr的一个索引
	int new_proto(const ByteArray& buffer);
	int delete_proto(int sproto_ptr);
	int save_proto(int sproto_ptr, int saveindex);
	int load_proto(int saveindex);
	int dump_proto(int sproto_ptr);
	//返回一个struct sproto_type_ptr的索引
	int query_type(int sproto_ptr, const String& type_name);
	Array protocol(int sproto_ptr, const Variant& tag);
	Dictionary get_default(int sproto_type_ptr);
	Array decode(int sproto_type_ptr, const ByteArray& buffer);
	ByteArray encode(int sproto_type_ptr, const Dictionary& data);
	ByteArray pack(const ByteArray& buffer);
	ByteArray unpack(const ByteArray& buffer);

protected:
	static void _bind_methods();
	int set_sproto_ptr(struct sproto* ptr);
	int set_sproto_type_ptr(struct sproto_type* ptr);
	struct sproto* get_sproto_ptr(int sproto_ptr);
	struct sproto_type* get_sproto_type_ptr(int sproto_type_ptr);
	void del_sproto_ptr(int sproto_ptr);
	void del_sproto_type_ptr(int sproto_type_ptr);
	void clear_sprotos();
private:
	OBJ_TYPE(Sproto, Reference);
	struct sproto *    g_sproto[MAX_GLOBALSPROTO];
	int                m_cacheSprotoIndex;
	int                m_cacheSprotoTypeIndex;
	SprotoIndexMap     m_cacheSprotoIndexMap; // struct sproto* : index
	IndexSprotoMap     m_cacheIndexSprotoMap; // index : struct sproto*
	SprotoTypeIndexMap m_cacheSprotoTypeIndexMap; // struct sproto_type* : index  
	IndexSprotoTypeMap m_cacheIndexSprotoTypeMap; //index : struct sproto_type*
	ByteArray          m_buffer;                  //tmp alloc buffer
};

#endif
