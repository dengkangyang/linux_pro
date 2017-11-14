#include <iostream>
#include "json.hpp"
#include <string>
#include <vector>


using json = nlohmann::json;
int main()
{
	//key-value
	json j;
	j["pi"] = 3.14;
	j["happy"] = true;
	j["name"] = "Niels";
	j["nothing"] = nullptr;
	j["answer"]["everything"] = 42;
	j["list"] = { 1, 0, 2 };
	j["object"] = { { "currency", "USD" }, { "value", 42.99 } };
	
	std::cout << j.dump() << std::endl;


	//json::array json::object
	// a way to express the empty array []
	json empty_array_explicit = json::array();

	// ways to express the empty object {}
	json empty_object_implicit = json({});
	json empty_object_explicit = json::object();

	// a way to express an _array_ of key/value pairs [["currency", "USD"], ["value", 42.99]]
	json array_not_object = { json::array({ "currency", "USD"}), json::array({ "value", 42.99 }) };
	for (auto object:array_not_object)
	{
		std::cout << object << std::endl;
	}
	std::cout << array_not_object << std::endl;

	//string转换为json
	json j1 = "{ \"happy\": true, \"pi\": 3.141 }"_json;
	json j2 = R"({"happy": true,"pi": 3.141})"_json;
	auto j3 = json::parse("{ \"happy\": true, \"pi\": 3.141 }");
	std::cout << j1 << std::endl;
	std::cout << j2 << std::endl;
	std::cout << j3 << std::endl;

	//json转换为string
	std::string s = j.dump();
	std::cout << s << std::endl;
	std::cout << j.dump(4) << std::endl;   //缩进4个字节

	//与STL适
	json jt;
	// create an array using push_back
	jt.push_back("foo");
	jt.push_back(1);
	jt.push_back(true);
	
	// also use emplace_back
	jt.emplace_back(1.78);
	// iterate the array
	for (json::iterator it = jt.begin(); it != jt.end(); ++it) {
		std::cout << *it << '\n';
	}

	// range-based for
	for (auto& element : jt) {
		std::cout << element << '\n';
	}

	// getter/setter
	const std::string tmp = jt[0];
	jt[1] = 42;
	bool foo = jt.at(2);

	// comparison
	std::cout << (jt == "[\"foo\", 1, true]"_json) << std::endl;;  // true

	std::cout << jt << std::endl;
	// other stuff
	j.size();     // 3 entries
	j.empty();    // false
	j.type();     // json::value_t::array
	j.clear();    // the array is empty again

	// convenience type checkers
	j.is_null();
	j.is_boolean();
	j.is_number();
	j.is_object();
	j.is_array();
	j.is_string();


	// create an object
	json o;
	o["foo"] = 23;
	o["bar"] = false;
	o["baz"] = 3.141;

	// also use emplace
	o.emplace("weather", "sunny");

	// special iterator member functions for objects
	for (json::iterator it = o.begin(); it != o.end(); ++it) {
		std::cout << it.key() << " : " << it.value() << "\n";
	}

	// find an entry
	if (o.find("foo") != o.end()) {
		// there is an entry with key "foo"
	}

	// or simpler using count()
	int foo_present = o.count("foo"); // 1
	int fob_present = o.count("fob"); // 0

	// delete an entry
	o.erase("foo");


	//Binary formats(CBOR and MessagePack)
	// create a JSON value
	json jf = R"({"compact": true, "schema": 0})"_json;
	// serialize to CBOR
	std::vector<uint8_t> v_cbor = json::to_cbor(jf);
	for (int i = 0; i < v_cbor.size(); i++)
	{
		std::cout << std::hex << int(v_cbor[i])<<" ";
	}
	std::cout <<std::endl;

	// roundtrip
	json j_from_cbor = json::from_cbor(v_cbor);
	std::cout << j_from_cbor << std::endl;


	// serialize to MessagePack
	std::vector<uint8_t> v_msgpack = json::to_msgpack(jf);
	// 0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74, 0xc3, 0xa6, 0x73, 0x63, 0x68, 0x65, 0x6d, 0x61, 0x00
	for (int i = 0; i < v_msgpack.size(); i++)
	{
		std::cout << std::hex << int(v_msgpack[i]) << " ";
	}
	std::cout << std::endl;
	// roundtrip
	json j_from_msgpack = json::from_msgpack(v_msgpack);
	std::cout << j_from_msgpack << std::endl;

	return 0;
}