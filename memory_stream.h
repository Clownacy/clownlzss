// Copyright (c) 2018 Clownacy

#pragma once 

#include <stddef.h>

typedef struct MemoryStream MemoryStream;

MemoryStream* MemoryStream_Init(size_t growth);
void MemoryStream_WriteByte(MemoryStream *memory_stream, unsigned char byte);
void MemoryStream_WriteBytes(MemoryStream *memory_stream, unsigned char *bytes, unsigned int byte_count);
unsigned char* MemoryStream_GetBuffer(MemoryStream *memory_stream);
size_t MemoryStream_GetIndex(MemoryStream *memory_stream);
void MemoryStream_Reset(MemoryStream *memory_stream);
