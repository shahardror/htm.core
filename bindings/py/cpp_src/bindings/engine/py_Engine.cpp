/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2018, Numenta, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * Author: @chhenning, 2018
 * --------------------------------------------------------------------- */

/** @file
PyBind11 bindings for Engine classes
*/


#include <bindings/suppress_register.hpp>  //include before pybind11.h
#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>


#include <htm/os/OS.hpp>
#include <htm/os/Timer.hpp>

#include <htm/ntypes/Array.hpp>

#include <htm/engine/Link.hpp>
#include <htm/engine/Network.hpp>
#include <htm/engine/Region.hpp>
#include <htm/engine/Spec.hpp>
#include <plugin/PyBindRegion.hpp>
#include <plugin/RegisteredRegionImplPy.hpp>

namespace py = pybind11;
using namespace htm;

namespace htm_ext
{

	typedef std::shared_ptr<Region> Region_Ptr_t;

    void init_Engine(py::module& m)
    {
        ///////////////////
        // Dimensions
        ///////////////////

        py::class_<Dimensions> py_Dimensions(m, "Dimensions");

        // constructors
        py_Dimensions.def(py::init<>())
            .def(py::init<std::vector<UInt>>())
            .def(py::init<UInt>())
            .def(py::init<UInt, UInt>())
            .def(py::init<UInt, UInt, UInt>());

        // members
        py_Dimensions.def("getCount", &Dimensions::getCount)
            .def("size", &Dimensions::size)
            .def("isUnspecified", &Dimensions::isUnspecified)
            .def("isDontcare", &Dimensions::isDontcare)
            .def("isSpecified", &Dimensions::isSpecified)
            .def("isInvalid", &Dimensions::isInvalid)
            .def("toString", &Dimensions::toString, "", py::arg("humanReadable") = true)
            ;

        // operator overloading
        py_Dimensions.def(py::self == py::self)
            .def(py::self != py::self);

        // python slots
        py_Dimensions.def("__str__", &Dimensions::toString)
            .def("__repr__", &Dimensions::toString);

        ///////////////////
        // Link
        ///////////////////
        py::class_<Link, std::shared_ptr<Link>> py_Link(m, "Link");

        // constructors
        py_Link.def(py::init<>())
            .def(py::init<const std::string&, const std::string&
                , const std::string&, const std::string&
                , const std::string&, const std::string&
                , size_t>()
                , ""
                , py::arg("linkType"), py::arg("linkParams")
                , py::arg("srcRegionName"), py::arg("destRegionName")
                , py::arg("srcOutputName") = "", py::arg("destInputName") = ""
                , py::arg("propagationDelay") = 0);

				// member functions
        py_Link.def("toString", &Link::toString);
        py_Link.def("getDestRegionName", &Link::getDestRegionName);
        py_Link.def("getSrcRegionName",  &Link::getSrcRegionName);
        py_Link.def("getSrcOutputName",  &Link::getSrcOutputName);
        py_Link.def("getDestInputName",  &Link::getDestInputName);
        py_Link.def("getLinkType",       &Link::getLinkType);



        ///////////////////
        // Spec
        ///////////////////
        py::class_<Spec> py_Spec(m, "Spec");

        ///////////////////
        // Region
        ///////////////////

        py::class_<Region, std::shared_ptr<Region>> py_Region(m, "Region");

        py_Region.def("getName", &Region::getName)
            .def("getType", &Region::getType)
            .def("getDimensions", &Region::getDimensions)
            .def("setDimensions", &Region::setDimensions)
			.def("getOutputElementCount", &Region::getNodeOutputElementCount);

		py_Region.def("getParameterInt32", &Region::getParameterInt32)
		    .def("getParameterUInt32", &Region::getParameterUInt32)
			.def("getParameterInt64",  &Region::getParameterInt64)
			.def("getParameterUInt64", &Region::getParameterUInt64)
			.def("getParameterReal32", &Region::getParameterReal32)
			.def("getParameterReal64", &Region::getParameterReal64)
			.def("getParameterBool",   &Region::getParameterBool)
			.def("getParameterString", &Region::getParameterString);

        py_Region.def("setParameterInt32", &Region::setParameterInt32)
		    .def("setParameterUInt32", &Region::setParameterUInt32)
			.def("setParameterInt64",  &Region::setParameterInt64)
			.def("setParameterUInt64", &Region::setParameterUInt64)
			.def("setParameterReal32", &Region::setParameterReal32)
			.def("setParameterReal64", &Region::setParameterReal64)
			.def("setParameterBool",   &Region::setParameterBool)
			.def("setParameterString", &Region::setParameterString);

// TODO:		py_Region.def("getParameterArray", [](Region& r, const std::string &name, Array &array
//			.def("setParameterArray",

        py_Region.def("__setattr__", [](Region& r, const std::string& Name, py::dict& d)
        {
            //r.python_attributes.insert(std::pair<std::string, py::object>(Name, d));
        });


        py_Region.def("__setattr__", [](Region& r, py::args args)
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                auto arg = args[i];
                std::string as_string = py::str(arg.get_type());

                if (py::isinstance<py::str>(arg))
                {
                    auto str = arg.cast<std::string>();
                }
                else if (py::isinstance<py::dict>(arg))
                {
                    auto dict = arg.cast<std::map<std::string, std::string>>();
                }
            }
        });



        py_Region.def("__getattr__", [](const Region& r, py::args args)
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                auto arg = args[i];
                std::string as_string = py::str(arg.get_type());

                if (py::isinstance<py::str>(arg))
                {
                    std::stringstream ss;
                    ss << "Attribute " << arg.cast<std::string>() << " not found";

                    throw std::runtime_error(ss.str());
                }
                else
                {
                    throw std::runtime_error("Unknown attribute.");
                }
            }
        });


        /*

        getSpec

        static member
        getSpecFromType
        */

		// TODO: do we need a function like this?
        //py_Region.def("getSelf", [](const Region& self)
        //{
        //    return self.getParameterHandle("self");
        //});

        py_Region.def("getInputArray", [](const Region& self, const std::string& name)
        {
            auto array_ref = self.getInputData(name);

            return py::array_t<htm::Byte>();
        });

        py_Region.def("getOutputArray", [](const Region& self, const std::string& name)
        {
            auto array_ref = self.getOutputData(name);

            return py::array_t<htm::Byte>();
        });


        ///////////////////
        // Network
        ///////////////////

        py::class_<Network> py_Network(m, "Network");

        // constructors
        py_Network.def(py::init<>())
            .def(py::init<std::string>());


		py_Network.def("addRegion",
			(Region_Ptr_t (htm::Network::*)(
					const std::string&,
  					const std::string&,
                    const std::string&))
					&htm::Network::addRegion,
					"Normal add region."
					, py::arg("name")
					, py::arg("nodeType" )
					, py::arg("nodeParams"));

    py_Network.def("getRegions", &htm::Network::getRegions)
            .def("getRegion",          &htm::Network::getRegion)
            .def("getLinks",           &htm::Network::getLinks)
            .def("getMinPhase",        &htm::Network::getMinPhase)
            .def("getMaxPhase",        &htm::Network::getMaxPhase)
            .def("setMinEnabledPhase", &htm::Network::getMinPhase)
            .def("setMaxEnabledPhase", &htm::Network::getMaxPhase)
            .def("getMinEnabledPhase", &htm::Network::getMinPhase)
            .def("getMaxEnabledPhase", &htm::Network::getMaxPhase)
			.def("run",                &htm::Network::run);

        py_Network.def("initialize", &htm::Network::initialize);

     py_Network.def("save",            &htm::Network::save)
		           .def("load",            &htm::Network::load)
							 .def("saveToFile",      &htm::Network::saveToFile)
							 .def("loadFromFile",    &htm::Network::loadFromFile);

        py_Network.def("link", &htm::Network::link
            , "Defines a link between regions"
            , py::arg("srcName"), py::arg("destName")
            , py::arg("linkType") = "", py::arg("linkParams") = ""
            , py::arg("srcOutput") = "", py::arg("destInput") = ""
						, py::arg("propagationDelay") = 0);


        // plugin registration
        //     (note: we are re-directing these to static functions on the PyBindRegion class)
		//     (node: the typeName is "py."+className )
        py_Network.def_static("registerPyRegion",
		                 [](const std::string& module,
							const std::string& className) {
				htm::RegisteredRegionImplPy::registerPyRegion(module, className);
			});

        py_Network.def_static("unregisterPyRegion",
			             [](const std::string& typeName) {
				htm::RegisteredRegionImplPy::unregisterPyRegion(typeName);
			});
		py_Network.def_static("cleanup", &htm::Network::cleanup);



        ///////////////////
        // Collection
        ///////////////////

        // Regions
        typedef Collection<std::shared_ptr<Region>> Region_Collection_t;
        py::class_<Region_Collection_t> py_RegionCollection(m, "RegionCollection");
        py_RegionCollection.def("getByName", &Region_Collection_t::getByName);
        py_RegionCollection.def("contains", &Region_Collection_t::contains);
        py_RegionCollection.def("getCount", &Region_Collection_t::getCount);
        py_RegionCollection.def("size", &Region_Collection_t::size);

        // bare bone sequence protocol
        py_RegionCollection.def("__len__", &Region_Collection_t::getCount);
        py_RegionCollection.def("__getitem__", [](Region_Collection_t& coll, size_t i)
        {
            if (i >= coll.getCount())
            {
                throw py::index_error();
            }

            return coll.getByIndex(i);
        });

        // Links
        typedef std::vector<std::shared_ptr<Link>> Links_t;
        py::class_<Links_t> py_LinkCollection(m, "Links_t");
        py_LinkCollection.def("getCount", &Links_t::size);

        // bare bone sequence protocol
        py_LinkCollection.def("__len__", &Links_t::size);
        py_LinkCollection.def("__getitem__", [](Links_t& coll, size_t i)
        {
            if (i >= coll.size())
            {
                throw py::index_error();
            }

            return coll[i];
        });

        // not sure we need __iter__
        py_LinkCollection.def("__iter__", [](Links_t& coll) { return py::make_iterator(coll.begin(), coll.end()); }, py::keep_alive<0, 1>());
    }

} // namespace htm_ext
