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
#include "qoremagic.h"


static void magic_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink);
static void magic_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink);
static void magic_module_delete();

extern "C" DLLEXPORT void magic_qore_module_desc(QoreModuleInfo& mod_info) {
    mod_info.name = "magic";
    mod_info.version = PACKAGE_VERSION;
    mod_info.desc = "libmagic wrapper";
    mod_info.author = "Petr Vanek";
    mod_info.url = "http://qore.org";
    mod_info.api_major = QORE_MODULE_API_MAJOR;
    mod_info.api_minor = QORE_MODULE_API_MINOR;
    mod_info.init = magic_module_init;
    mod_info.ns_init = magic_module_ns_init;
    mod_info.del = magic_module_delete;
    mod_info.license = QL_LGPL;
    mod_info.license_str = "LGPL";
}

static QoreNamespace MNS("Qore::Magic");

static void magic_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink) {
    MNS.addSystemClass(initMagicClass(MNS));
}

static void magic_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink) {
    qns->addInitialNamespace(MNS.copy());
}

void magic_module_delete() {
    // nothing to do here in this case
}

