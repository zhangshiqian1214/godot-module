##godot-sproto模块
=================
sproto是云风发布的一个类似protobuf序列化库,支持integer, boolean, string, struct , array, map这几种类型
在脚本语言里面做序列化会相对方便
下载
-------
git clone https://github.com/zhangshiqian1214/godot-module.git
然后把对应模块的文件夹放到godot引擎相应目录下就可以了
使用方式:

```lua
var sproto
var sprotoObj
func _ready():
    sproto = Sproto.new()
	var f = File.new()
	if f.open("res://c2s.spb", File.READ) == OK :
	   var filebuf = f.get_buffer(f.get_len())
	   sprotoObj = sproto.new_proto(buffer)
	   sproto.save_proto(sprotoObj, 1)
	pass
	
func _on_test_pressed():
	var typeObj = sproto.query_type(sprotoObj, "Person")
	var defaultData = sproto.get_default(typeObj)
	print("defaultData:", defaultData)
	
	var person = {
		name = "Alice", 
		id = 10000, 
		cash = 3.1415926,
		myphone = {"number": "01234567890", "type": 3},
		phone = [
			{ "number": "123456789", "type": 1 },
			{ "number": "87654321", "type": 2 },
		],
		skills = [1, 2, 3, 4],
		actvityNames = [],
		taskIds = [],
	}
	
	var buf = sproto.encode(typeObj, person)
	print("buf.sz:", buf.size())
	
	var outdata = sproto.decode(typeObj, buf)
	print("decode outdata:", outdata)
	
	var packbuf = sproto.pack(buf)
	print("packbuf.sz:", packbuf.size())
	
	var unpackBuf = sproto.unpack(packbuf)
	print("unpackBuf.sz:", unpackBuf.size())
