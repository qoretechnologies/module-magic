/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    libmagic Qore wrapper

    Qore Programming Language

    Copyright 2012 - 2026 Qore Technologies, s.r.o.

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

#include <cstring>
#include <strings.h>  // strncasecmp


const TypedHashDecl* hashdeclMagicFileInfo = nullptr;


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


// Extracts buffer pointer and length from a QoreValue (string or binary).
// Returns false and raises an exception on invalid type.
static bool extractBufferData(QoreValue data, const char* func_name, const void*& buf, size_t& len,
        ExceptionSink* xsink) {
    qore_type_t qt = data.getType();
    if (qt == NT_BINARY) {
        const BinaryNode* s = data.get<const BinaryNode>();
        buf = s->getPtr();
        len = s->size();
    } else if (qt == NT_STRING) {
        const QoreStringNode* s = data.get<const QoreStringNode>();
        buf = s->c_str();
        len = s->size();
    } else {
        xsink->raiseException("MAGIC-ERROR",
            "%s requires 'data' argument: string or binary. Got: %s",
            func_name, data.getTypeName());
        return false;
    }
    return true;
}


// Parses a MIME string like "type/subtype; charset=encoding" into components.
// Extracts the charset= parameter specifically; any non-charset parameters are ignored.
static void parseMimeString(const char* mime_str, QoreStringNode*& mime_type, QoreStringNode*& mime_encoding) {
    if (!mime_str) {
        mime_type = new QoreStringNode("application/octet-stream");
        mime_encoding = new QoreStringNode("binary");
        return;
    }

    // Find the semicolon separator between MIME type and parameters
    const char* semi = strchr(mime_str, ';');
    if (!semi) {
        mime_type = new QoreStringNode(mime_str);
        mime_encoding = new QoreStringNode("binary");
        return;
    }

    mime_type = new QoreStringNode(mime_str, semi - mime_str);

    // Search for "charset=" parameter among potentially multiple parameters
    const char* p = semi + 1;
    while (*p) {
        // Skip whitespace
        while (*p == ' ') {
            ++p;
        }
        if (strncasecmp(p, "charset=", 8) == 0) {
            p += 8;
            // Extract the charset value (up to next ';' or end of string)
            const char* end = strchr(p, ';');
            if (end) {
                mime_encoding = new QoreStringNode(p, end - p);
            } else {
                mime_encoding = new QoreStringNode(p);
            }
            return;
        }
        // Skip to next parameter
        const char* next_semi = strchr(p, ';');
        if (!next_semi) {
            break;
        }
        p = next_semi + 1;
    }

    // No charset= parameter found
    mime_encoding = new QoreStringNode("binary");
}

// Builds a MagicFileInfo hash from description and MIME strings
static QoreHashNode* buildMagicFileInfo(const char* description, const char* mime_str, ExceptionSink* xsink) {
    if (!hashdeclMagicFileInfo) {
        xsink->raiseException("MAGIC-ERROR", "MagicFileInfo hashdecl not initialized");
        return nullptr;
    }

    QoreStringNode* mime_type = nullptr;
    QoreStringNode* mime_encoding = nullptr;
    parseMimeString(mime_str, mime_type, mime_encoding);

    // Derive is_text from mime_type
    bool is_text = (strncmp(mime_type->c_str(), "text/", 5) == 0);

    // Derive type_category (part before "/")
    QoreStringNode* type_category;
    const char* slash = strchr(mime_type->c_str(), '/');
    if (slash) {
        type_category = new QoreStringNode(mime_type->c_str(), slash - mime_type->c_str());
    } else {
        type_category = new QoreStringNode(mime_type->c_str());
    }

    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclMagicFileInfo, xsink), xsink);
    if (*xsink) {
        mime_type->deref();
        mime_encoding->deref();
        type_category->deref();
        return nullptr;
    }

    h->setKeyValue("description", new QoreStringNode(description ? description : ""), xsink);
    h->setKeyValue("mime_type", mime_type, xsink);
    h->setKeyValue("mime_encoding", mime_encoding, xsink);
    h->setKeyValue("is_text", is_text, xsink);
    h->setKeyValue("type_category", type_category, xsink);

    return h.release();
}

// Shared implementation: get structured file info (used by class method and free function)
// Caller is responsible for I/O interrupt and sandbox checks.
static QoreHashNode* fileInfoCore(const char* fileName, ExceptionSink* xsink) {
    // Use MAGIC_ERROR to ensure proper error reporting for non-existent files etc.
    MagicHelper descMagic(MAGIC_NONE | MAGIC_ERROR, xsink);
    if (*xsink || !descMagic.isValid()) {
        return nullptr;
    }

    const char* desc = descMagic.file(fileName);
    if (!desc) {
        if (!descMagic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine file type for '%s'", fileName);
        }
        return nullptr;
    }

    MagicHelper mimeMagic(MAGIC_MIME | MAGIC_ERROR, xsink);
    if (*xsink || !mimeMagic.isValid()) {
        return nullptr;
    }

    const char* mime = mimeMagic.file(fileName);
    if (!mime) {
        if (!mimeMagic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine MIME type for '%s'", fileName);
        }
        return nullptr;
    }

    return buildMagicFileInfo(desc, mime, xsink);
}

// Shared implementation: get structured buffer info (used by class method and free function)
// Caller is responsible for I/O interrupt check and data extraction.
static QoreHashNode* bufferInfoCore(const void* buf, size_t len, ExceptionSink* xsink) {
    // Use MAGIC_ERROR to ensure proper error reporting
    MagicHelper descMagic(MAGIC_NONE | MAGIC_ERROR, xsink);
    if (*xsink || !descMagic.isValid()) {
        return nullptr;
    }

    const char* desc = descMagic.buffer(buf, len);
    if (!desc) {
        if (!descMagic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine buffer type");
        }
        return nullptr;
    }

    MagicHelper mimeMagic(MAGIC_MIME | MAGIC_ERROR, xsink);
    if (*xsink || !mimeMagic.isValid()) {
        return nullptr;
    }

    const char* mime = mimeMagic.buffer(buf, len);
    if (!mime) {
        if (!mimeMagic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine buffer MIME type");
        }
        return nullptr;
    }

    return buildMagicFileInfo(desc, mime, xsink);
}

// Shared implementation: get buffer MIME type string
// Caller is responsible for I/O interrupt check and data extraction.
static QoreStringNode* bufferMimeTypeCore(const void* buf, size_t len, ExceptionSink* xsink) {
    MagicHelper magic(MAGIC_MIME_TYPE | MAGIC_ERROR, xsink);
    if (*xsink || !magic.isValid()) {
        return nullptr;
    }

    const char* ret = magic.buffer(buf, len);
    if (!ret) {
        if (!magic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine buffer MIME type");
        }
        return nullptr;
    }

    return new QoreStringNode(ret);
}


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

    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    // Check filesystem sandbox access before reading file
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(fileName->c_str(), QSEC_READ, xsink)) {
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

    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    const void* buf;
    size_t len;
    if (!extractBufferData(data, "Magic::buffer", buf, len, xsink)) {
        return nullptr;
    }

    MagicHelper magic(flags, xsink);

    if (*xsink || !magic.isValid()) {
        return nullptr;
    }

    const char* ret = magic.buffer(buf, len);

    if (!ret) {
        if (!magic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine buffer type");
        }
        return nullptr;
    }

    return new QoreStringNode(ret);
}

QoreHashNode* QoreMagic::fileInfo(const QoreStringNode* fileName, ExceptionSink* xsink) {
    AutoLocker al(m_lock);

    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    // Check filesystem sandbox access before reading file
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(fileName->c_str(), QSEC_READ, xsink)) {
        return nullptr;
    }

    return fileInfoCore(fileName->getBuffer(), xsink);
}

QoreHashNode* QoreMagic::bufferInfo(QoreValue data, ExceptionSink* xsink) {
    AutoLocker al(m_lock);

    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    const void* buf;
    size_t len;
    if (!extractBufferData(data, "Magic::bufferInfo", buf, len, xsink)) {
        return nullptr;
    }

    return bufferInfoCore(buf, len, xsink);
}

// Free function implementations for module-level convenience functions

QoreHashNode* magic_file_info_impl(const QoreStringNode* fileName, ExceptionSink* xsink) {
    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    // Check filesystem sandbox access
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(fileName->c_str(), QSEC_READ, xsink)) {
        return nullptr;
    }

    return fileInfoCore(fileName->getBuffer(), xsink);
}

QoreStringNode* magic_mime_type_impl(const QoreStringNode* fileName, ExceptionSink* xsink) {
    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    // Check filesystem sandbox access
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(fileName->c_str(), QSEC_READ, xsink)) {
        return nullptr;
    }

    MagicHelper magic(MAGIC_MIME_TYPE | MAGIC_ERROR, xsink);
    if (*xsink || !magic.isValid()) {
        return nullptr;
    }

    const char* ret = magic.file(fileName->getBuffer());
    if (!ret) {
        if (!magic.checkException(xsink)) {
            xsink->raiseException("MAGIC-ERROR", "Failed to determine MIME type for '%s'",
                fileName->c_str());
        }
        return nullptr;
    }

    return new QoreStringNode(ret);
}

QoreHashNode* magic_buffer_info_impl(QoreValue data, ExceptionSink* xsink) {
    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    const void* buf;
    size_t len;
    if (!extractBufferData(data, "magic_buffer_info()", buf, len, xsink)) {
        return nullptr;
    }

    return bufferInfoCore(buf, len, xsink);
}

QoreStringNode* magic_buffer_mime_type_impl(QoreValue data, ExceptionSink* xsink) {
    // Check for I/O interrupt before starting
    if (qore_check_cancel(xsink)) {
        return nullptr;
    }

    const void* buf;
    size_t len;
    if (!extractBufferData(data, "magic_buffer_mime_type()", buf, len, xsink)) {
        return nullptr;
    }

    return bufferMimeTypeCore(buf, len, xsink);
}
