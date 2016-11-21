
extends Panel

# member variables here, example:
# var a=2
# var b="textvar"

var sproto
var c2s_sproto_ptr

func _ready():
	# Called every time the node is added to the scene.
	# Initialization here
	var f = File.new()
	sproto = Sproto.new()
	if f.open("res://c2s.spb", File.READ) == OK :
		var buffer = f.get_buffer(f.get_len())
		c2s_sproto_ptr = sproto.new_proto(buffer)
		print("c2s_sproto_ptr:", c2s_sproto_ptr)
		sproto.save_proto(c2s_sproto_ptr, 1)
	
	pass


func _on_ChooseBtn_pressed():
	#get_tree().change_scene("res://choose.tscn")
	
	pass # replace with function body


func _on_Gen_pressed():
	var type_ptr = sproto.query_type(c2s_sproto_ptr, "Person")
	print("Person type_ptr:", type_ptr)
	var data = sproto.get_default(type_ptr)
	print("data:", data)
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
	var buf = sproto.encode(type_ptr, person)
	print("buf.sz:", buf.size())

	var outdata = sproto.decode(type_ptr, buf)
	print("decode outdata:", outdata)
	
	var packbuf = sproto.pack(buf)
	print("packbuf.sz:", packbuf.size())
	
	var unpackBuf = sproto.unpack(packbuf)
	print("unpackBuf.sz:", unpackBuf.size())

	pass # replace with function body
