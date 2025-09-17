// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License


#include "lua_api/l_clientobject.h"
#include "l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "client/client.h"
#include "object_properties.h"
#include "util/pointedthing.h"

using object_t = u16;

ClientActiveObject *ClientObjectRef::getClientActiveObject()
{
	return m_object;
}

ClientObjectRef *ClientObjectRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *userdata = luaL_checkudata(L, narg, className);
	if (!userdata)
		luaL_typerror(L, narg, className);
	return *(ClientObjectRef **)userdata;
}

ClientActiveObject *ClientObjectRef::get_cao(ClientObjectRef *ref)
{
	ClientActiveObject *obj = ref->m_object;
	return obj;
}

GenericCAO *ClientObjectRef::get_generic_cao(ClientObjectRef *ref, lua_State *L)
{
	ClientActiveObject *obj = get_cao(ref);
	if (!obj)
		return nullptr;
	ClientEnvironment &env = getClient(L)->getEnv();
	GenericCAO *gcao = env.getGenericCAO(obj->getId());
	return gcao;
}

int ClientObjectRef::l_get_pos(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	ClientActiveObject *cao = get_cao(ref);
	if (!cao)
		return 0;
	push_v3f(L, cao->getPosition() / BS);
	return 1;
}

int ClientObjectRef::l_set_pos(lua_State *L)
{
    ClientObjectRef *ref = checkobject(L, 1);
    ClientActiveObject *cao = get_cao(ref);
    if (!cao)
        return 0;

    v3f pos = check_v3f(L, 2);
    cao->setPosition(pos * BS);

    return 0;
}

// set_attachment(self, parent_obj_id, parent_bone_name, position, rotation, force_visible)
int ClientObjectRef::l_set_attachment(lua_State *L)
{
    ClientObjectRef *ref = checkobject(L, 1);
    ClientActiveObject *cao = get_cao(ref);
    if (!cao)
        return 0;

    object_t parent_id = (object_t)luaL_checkinteger(L, 2);

    const char *bone_cstr = luaL_checkstring(L, 3);
    std::string bone(bone_cstr);

    v3f pos = check_v3f(L, 4) * BS;

    v3f rot = check_v3f(L, 5);

    bool force_visible = lua_toboolean(L, 6);

    cao->setAttachment(parent_id, bone, pos, rot, force_visible);

    return 0;
}

int ClientObjectRef::l_get_velocity(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	push_v3f(L, gcao->getVelocity() / BS);
	return 1;
}

int ClientObjectRef::l_get_acceleration(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	push_v3f(L, gcao->getAcceleration() / BS);
	return 1;
}

int ClientObjectRef::l_get_rotation(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	push_v3f(L, gcao->getRotation());
	return 1;
}

int ClientObjectRef::l_is_player(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	lua_pushboolean(L, gcao->isPlayer());
	return 1;
}

int ClientObjectRef::l_is_local_player(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	lua_pushboolean(L, gcao->isLocalPlayer());
	return 1;
}

int ClientObjectRef::l_get_name(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	lua_pushstring(L, gcao->getName().c_str());
	return 1;
}

int ClientObjectRef::l_get_attach(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	ClientActiveObject *parent = gcao->getParent();
	if (!parent)
		return 0;
	push_objectRef(L, parent->getId());
	return 1;
}

int ClientObjectRef::l_get_properties(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	const ObjectProperties* prop = &gcao->getProperties();
	push_object_properties(L, prop);
	return 1;	
}


int ClientObjectRef::l_set_properties(lua_State *L)
{
    ClientObjectRef *ref = checkobject(L, 1);
    GenericCAO *gcao = get_generic_cao(ref, L);

    ObjectProperties prop = gcao->getProperties();

    read_object_properties(L, 2, nullptr, &prop, getClient(L)->idef());

    gcao->setProperties(prop);

    return 1;
}

int ClientObjectRef::l_get_id(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	GenericCAO *gcao = get_generic_cao(ref, L);
	if (!gcao)
		return 0;
	lua_pushnumber(L, gcao->getId());
	return 1;
}

int ClientObjectRef::l_remove(lua_State *L)
{
	ClientObjectRef *ref = checkobject(L, 1);
	ClientActiveObject *cao = get_cao(ref);
	if (!cao)
		return 0;
	getClient(L)->getEnv().removeActiveObject(cao->getId());

	return 0;
}

ClientObjectRef::ClientObjectRef(ClientActiveObject *object) : m_object(object)
{
}

void ClientObjectRef::create(lua_State *L, ClientActiveObject *object)
{
	if (!object) {
        lua_pushnil(L);
        return;
    }
	ClientObjectRef *o = new ClientObjectRef(object);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ClientObjectRef::create(lua_State *L, s16 id)
{
	create(L, ((ClientEnvironment *)getEnv(L))->getActiveObject(id));
}

void ClientObjectRef::set_null(lua_State *L)
{
	ClientObjectRef *obj = checkobject(L, -1);
	obj->m_object = nullptr;
}

int ClientObjectRef::gc_object(lua_State *L)
{
	ClientObjectRef *obj = *(ClientObjectRef **)(lua_touserdata(L, 1));
	delete obj;
	return 0;
}

// taken from LuaLocalPlayer
void ClientObjectRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable); // hide metatable from lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1); // Drop metatable

	luaL_openlib(L, 0, methods, 0); // fill methodtable
	lua_pop(L, 1);			// Drop methodtable
}

const char ClientObjectRef::className[] = "ClientObjectRef";
luaL_Reg ClientObjectRef::methods[] = {
		luamethod(ClientObjectRef, get_pos),
		luamethod(ClientObjectRef, set_pos),
		luamethod(ClientObjectRef, set_attachment),
		luamethod(ClientObjectRef, get_velocity),
		luamethod(ClientObjectRef, get_acceleration),
		luamethod(ClientObjectRef, get_rotation),
		luamethod(ClientObjectRef, is_player),
		luamethod(ClientObjectRef, is_local_player),
		luamethod(ClientObjectRef, get_name),
		luamethod(ClientObjectRef, get_attach),
		luamethod(ClientObjectRef, get_properties),
		luamethod(ClientObjectRef, set_properties),
		luamethod(ClientObjectRef, get_id),
		luamethod(ClientObjectRef, remove),{0, 0}
	};