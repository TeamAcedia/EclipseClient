// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License


#include "lua_api/l_base.h"
#include "client/clientobject.h"
#include "client/content_cao.h"

class ClientObjectRef : public ModApiBase
{
public:
	ClientObjectRef(ClientActiveObject *object);

	~ClientObjectRef() = default;

	ClientActiveObject *getClientActiveObject();

	static void Register(lua_State *L);

	static void create(lua_State *L, ClientActiveObject *object);
	static void create(lua_State *L, s16 id);

	static void set_null(lua_State *L);

	static ClientObjectRef *checkobject(lua_State *L, int narg);

private:
	ClientActiveObject *m_object = nullptr;
	static const char className[];
	static luaL_Reg methods[];

	static ClientActiveObject *get_cao(ClientObjectRef *ref);
	static GenericCAO *get_generic_cao(ClientObjectRef *ref, lua_State *L);

	static int gc_object(lua_State *L);

	// get_pos(self)
	// returns: {x=num, y=num, z=num}
	static int l_get_pos(lua_State *L);
	
	// set_pos(self, pos)
	// requires: {x=num, y=num, z=num}
	static int l_set_pos(lua_State *L);

	// set_attachment(self, parent_obj_id, parent_bone_name, position, rotation, force_visible)
	static int l_set_attachment(lua_State *L);

	// get_velocity(self)
	static int l_get_velocity(lua_State *L);

	// get_acceleration(self)
	static int l_get_acceleration(lua_State *L);

	// get_rotation(self)
	static int l_get_rotation(lua_State *L);

	// is_player(self)
	static int l_is_player(lua_State *L);

	// is_local_player(self)
	static int l_is_local_player(lua_State *L);

	// get_name(self)
	static int l_get_name(lua_State *L);

	// get_attach(self)
	static int l_get_attach(lua_State *L);

	// get_nametag(self)
	static int l_get_nametag(lua_State *L);

	// get_item_textures(self)
	static int l_get_item_textures(lua_State *L);


	// get_properties(self)
	static int l_get_properties(lua_State *L);

	// set_properties(self, properties)
	static int l_set_properties(lua_State *L);

	// get_id(self)
	static int l_get_id(lua_State *L);

	// remove(self)
	static int l_remove(lua_State *L);
	
};