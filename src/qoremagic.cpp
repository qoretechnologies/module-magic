/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    libmagic Qore wrapper

    Qore Programming Language

    Copyright 2012 - 2022 Qore Technologies, s.r.o.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <qore/Qore.h>
#include <qore/QoreSandboxManager.h>
#include "qoremagic.h"


class MagicHelper
{
    magic_t m_magic;

public:
    MagicHelper(int flags, ExceptionSink *xsink) : m_magic(nullptr) {
        m_magic = magic_open(flags);
        if (m_magic == nullptr) {
            xsink->raiseException("MAGIC-ERROR", "Failed to initialize libmagic");
            return;
        }
        if (magic_load(m_magic, nullptr) == -1) {
            checkException(xsink);
        }
    }

    ~MagicHelper() {
        if (m_magic) {
            magic_close(m_magic);
        }
    }

    bool isValid() const {
        return m_magic != nullptr;
    }

    magic_t* magic() {
        return &m_magic;
    }

    const char* file(const char *fname) {
        return magic_file(m_magic, fname);
    }

    const char* buffer(const void *data, size_t len) {
        return magic_buffer(m_magic, data, len);
    }

    bool checkException(ExceptionSink *xsink) {
        if (!m_magic) {
            xsink->raiseException("MAGIC-ERROR", "Magic handle is not initialized");
            return true;
        }
        const char *errmsg = magic_error(m_magic);
        if (errmsg) {
            int err = magic_errno(m_magic);
            xsink->raiseException("MAGIC-ERROR", "ERR-%d: %s", err, errmsg);
            return true;
        }
        return false;
    }
};


QoreMagic::QoreMagic(ExceptionSink* xsink) : m_flags(MAGIC_NONE) {
    (void)xsink;
}

QoreMagic::QoreMagic(int flags, ExceptionSink* xsink) : m_flags(flags) {
    (void)xsink;
}

QoreMagic::~QoreMagic()
{
}

void QoreMagic::setFlags(int flags, ExceptionSink* xsink) {
    (void)xsink;
    AutoLocker al(m_lock);
    m_flags = flags;
}

int QoreMagic::getFlags() {
    AutoLocker al(m_lock);
    return m_flags;
}

AbstractQoreNode* QoreMagic::file(const QoreStringNode* fileName, ExceptionSink* xsink) {
    return file(fileName, m_flags, xsink);
}

AbstractQoreNode* QoreMagic::file(const QoreStringNode* fileName, int flags, ExceptionSink* xsink) {
    AutoLocker al(m_lock);

    // Check filesystem sandbox access before reading file
    QoreSandboxManager* sm = runtime_get_sandbox_manager();
    if (sm && !sm->checkFilesystemAccess(fileName->c_str(), QSEC_READ, xsink)) {
        return nullptr;
    }

    MagicHelper magic(flags, xsink);

    if (*xsink || !magic.isValid()) {
        return nullptr;
    }

    const char* ret = magic.file(fileName->getBuffer());

    if (!ret) {
        if (!magic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine file type for '%s'",
                fileName->c_str());
        }
        return nullptr;
    }

    return new QoreStringNode(ret);
}

AbstractQoreNode* QoreMagic::buffer(QoreValue data, ExceptionSink* xsink) {
    return buffer(data, m_flags, xsink);
}

AbstractQoreNode* QoreMagic::buffer(QoreValue data, int flags, ExceptionSink* xsink) {
    AutoLocker al(m_lock);

    MagicHelper magic(flags, xsink);

    if (*xsink || !magic.isValid()) {
        return nullptr;
    }

    qore_type_t qt = data.getType();
    const char* ret;
    if (qt == NT_BINARY) {
        const BinaryNode* s = data.get<const BinaryNode>();
        ret = magic.buffer(s->getPtr(), s->size());
    } else if (qt == NT_STRING) {
        const QoreStringNode* s = data.get<const QoreStringNode>();
        ret = magic.buffer(s->c_str(), s->size());
    } else {
        xsink->raiseException("MAGIC-ERROR",
            "Magic::buffer requires 'data' argument: string or binary. Got: %s",
            data.getTypeName());
        return nullptr;
    }

    if (!ret) {
        if (!magic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine buffer type");
        }
        return nullptr;
    }

    return new QoreStringNode(ret);
}

