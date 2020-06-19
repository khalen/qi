//
// Copyright (c) Quantum Immortality 2020
// Created by Jon Davis on 6/5/20.
//

#ifndef QED_PARSE_H
#define QED_PARSE_H

#include "basictypes.h"
#include "memory.h"
#include "stringtable.h"

// Returns nullptr on success, error message on failure; *ksp will be created if initially null
const char *QED_LoadBuffer(KeyStore **ksp, const char *ksName, const char *buf, size_t bufSize);
const char *QED_LoadFile(KeyStore **ksp, const char *ksName, const char *fileName); // Global data store interface
KeyStore *  QED_LoadDataStore(const char *dsName);
KeyStore *  QED_GetDataStore(const char *dsName);

#endif // QED_PARSE_H

