// -*- mode: cpp -*-
// =============================================================================
// @@-COPYRIGHT-START-@@
//
// Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// @@-COPYRIGHT-END-@@
// =============================================================================
#include "snpehelper.h"

PYBIND11_MODULE(snpehelper, m) {
    m.doc() = "snpehelper plugin";
    m.attr("__name__") = "snpehelper";
    m.attr("__version__") = "0.1.0";
    m.attr("__author__") = "Sumith Kumar Budha";
    m.attr("__license__") = "BSD 3-clause";

    py::class_<SnpeContext>(m, "SnpeContext")
        .def(py::init< const std::string&, const std::vector<std::string>&, std::vector<std::string>&, const std::vector<std::string>&, const string, const string, const bool >())
        .def("Initialize", &SnpeContext::Initialize)
        .def("SetInputBuffer", &SnpeContext::SetInputBuffer)
        .def("Execute", &SnpeContext::Execute)
        .def("GetOutputBuffer", &SnpeContext::GetOutputBuffer);
}