#pragma once

#define DISABLE_COPY_AND_ASSIGN(cls)	cls(const cls &obj) = delete;\
	cls& operator=(const cls &obj) = delete;\
	cls(cls &&obj) = delete;\
	cls& operator=(cls &&obj) = delete;

