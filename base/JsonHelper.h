#pragma once

#include "document.h"
#include "glog/logging.h"

static inline bool JsonGetStringMemberValue(rapidjson::Value &doc, const char *key, std::string &value)
{
	DCHECK(doc.IsObject());
	auto json_it = doc.FindMember(key);
	if (json_it == doc.MemberEnd())
		return false;

	auto *obj = &json_it->value;
	if (!obj->IsString())
	{
		return false;
	}
	value.assign(obj->GetString(), obj->GetStringLength());
	return true;
}

static inline bool JsonGetObjectMemberValue(rapidjson::Value &doc, const char* key, rapidjson::Value** val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	*val = &it->value;
	if (!(*val)->IsObject())
		return false;

	return true;
}

static inline bool JsonGetArrayMemberValue(rapidjson::Value &doc, const char* key, rapidjson::Value** val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	*val = &it->value;
	if (!(*val)->IsArray())
		return false;

	return true;
}

static inline bool JsonGetUIntMemberValue(rapidjson::Value &doc, const char* key, unsigned int &val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	auto *obj = &it->value;
	if (!obj->IsUint())
		return false;

	val = obj->GetUint();
	return true;
}

static inline bool JsonGetUInt64MemberValue(rapidjson::Value &doc, const char* key, uint64_t &val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	auto *obj = &it->value;
	if (!obj->IsUint64())
		return false;

	val = obj->GetUint64();
	return true;
}

static inline bool JsonGetIntMemberValue(rapidjson::Value &doc, const char* key, int &val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	auto *obj = &it->value;
	if (!obj->IsInt())
		return false;

	val = obj->GetInt();
	return true;
}

static inline bool JsonGetBoolMemberValue(rapidjson::Value &doc, const char* key, bool &val)
{
	DCHECK(doc.IsObject());
	auto it = doc.FindMember(key);
	if (it == doc.MemberEnd())
		return false;

	auto *obj = &it->value;
	if (!obj->IsBool())
		return false;

	val = obj->GetBool();
	return true;
}