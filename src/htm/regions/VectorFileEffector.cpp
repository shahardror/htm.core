/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013, Numenta, Inc.
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
 * --------------------------------------------------------------------- */

/** @file
 * Implementation for VectorFileEffector class
 */

#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>

#include <htm/engine/Output.hpp>
#include <htm/engine/Input.hpp>
#include <htm/engine/Region.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/regions/VectorFileEffector.hpp>
#include <htm/utils/Log.hpp>

namespace htm {

VectorFileEffector::VectorFileEffector(const ValueMap &params, Region* region)
    : RegionImpl(region), dataIn_(NTA_BasicType_Real32), filename_(""),
      outFile_(nullptr) {
  if (params.contains("outputFile")) {
    std::string s = params.getString("outputFile");
    openFile(s);
  }
  else
    filename_ = "";
}

VectorFileEffector::VectorFileEffector(ArWrapper& wrapper, Region* region)
    : RegionImpl(region), dataIn_(NTA_BasicType_Real32), filename_(""),
      outFile_(nullptr) {
  cereal_adapter_load(wrapper);
}


VectorFileEffector::~VectorFileEffector() { closeFile(); }

void VectorFileEffector::initialize() {
  NTA_CHECK(region_ != nullptr);
  // We have no outputs or parameters; just need our input.
  Input *in = region_->getInput("dataIn");
  NTA_ASSERT(in) << "VectorFileEffector::init - 'dataIn' input not configured\n";

  if (!in->hasIncomingLinks() || in->getData().getCount() == 0) {
    NTA_THROW << "VectorFileEffector::init - no input Data found\n";
  }
  dataIn_ = in->getData();
}

void VectorFileEffector::compute() {
  NTA_DEBUG << "VectorFileEffector compute() input: " << *region_->getInput("dataIn") << "\n";
  dataIn_ = region_->getInput("dataIn")->getData();
  // It's not necessarily an error to have no inputs. In this case we just
  // return
  if (dataIn_.getCount() == 0)
    return;

  // Don't write if there is no open file.
  if (outFile_ == nullptr) {
    NTA_WARN
        << "VectorFileEffector (Warning) compute() called, but there is no open file";
    return;
  }

  // Ensure we can write to it
  if (outFile_->fail()) {
    NTA_THROW << "VectorFileEffector: There was an error writing to the file "
              << filename_.c_str() << "\n";
  }


  Real *inputVec = (Real *)(dataIn_.getBuffer());
  NTA_CHECK(inputVec != nullptr);
  std::ofstream &outFile = *outFile_;
  for (Size offset = 0; offset < dataIn_.getCount(); ++offset) {
    if (offset == 0)
      outFile << inputVec[offset];
    else
      outFile << "," << inputVec[offset];
  }
  outFile << "\n";
}

void VectorFileEffector::closeFile() {
  if (outFile_) {
    outFile_->close();
    outFile_ = nullptr;
    filename_ = "";
  }
}

void VectorFileEffector::openFile(const std::string &filename) {

  if (outFile_ && !outFile_->fail())
    closeFile();
  if (filename == "")
    return;

  outFile_ = new std::ofstream(filename.c_str(), std::ios::app);
  if (outFile_->fail())
  {
    delete outFile_;
    outFile_ = nullptr;
    NTA_THROW
        << "VectorFileEffector::openFile -- unable to create or open file: "
        << filename.c_str();
  }
  filename_ = filename;
}

void VectorFileEffector::setParameterString(const std::string &paramName,
                                            Int64 index, const std::string &s) {

  if (paramName == "outputFile") {
    if (s == filename_ && outFile_)
      return; // already set
    if (outFile_)
      closeFile();
    openFile(s);
  } else {
    NTA_THROW << "VectorFileEffector -- Unknown string parameter " << paramName;
  }
}

std::string VectorFileEffector::getParameterString(const std::string &paramName,
                                                   Int64 index) {
  if (paramName == "outputFile") {
    return filename_;
  } else {
    NTA_THROW << "VectorFileEffector -- unknown parameter " << paramName;
  }
}

std::string
VectorFileEffector::executeCommand(const std::vector<std::string> &args,
                                   Int64 index) {
  NTA_CHECK(args.size() > 0);
  // Process the flushFile command
  if (args[0] == "flushFile") {
    // Ensure we have a valid file before flushing, otherwise fail silently.
    if (!((outFile_ == nullptr) || (outFile_->fail()))) {
      outFile_->flush();
    }
  } else if (args[0] == "closeFile") {
    closeFile();
  } else if (args[0] == "echo") {
    // Ensure we have a valid file before flushing, otherwise fail silently.
    if ((outFile_ == nullptr) || (outFile_->fail())) {
      NTA_THROW << "VectorFileEffector: echo command failed because there is "
                   "no file open";
    }

    for (size_t i = 1; i < args.size(); i++) {
      *outFile_ << args[i];
    }
    *outFile_ << "\n";
  } else {
    NTA_THROW << "VectorFileEffector: Unknown execute '" << args[0] << "'";
  }

  return "";
}

Spec *VectorFileEffector::createSpec() {

  auto ns = new Spec;
  ns->description =
      "VectorFileEffector is a node that simply writes its "
      "input vectors to a text file. The target filename is specified "
      "using the 'outputFile' parameter at run time. On each "
      "compute, the current input vector is written (but not flushed) "
      "to the file.\n";

  ns->inputs.add("dataIn",
              InputSpec("Data to be written to file",
                        NTA_BasicType_Real32,
                        0,     // count
                        false, // required?
                        true, // isRegionLevel
                        true   // isDefaultInput
                        ));

  ns->parameters.add("outputFile",
              ParameterSpec("Writes output vectors to this file on each "
                            "compute. Will append to any "
                            "existing data in the file. This parameter "
                            "must be set at runtime before "
                            "the first compute is called. Throws an "
                            "exception if it is not set or "
                            "the file cannot be written to.\n",
                            NTA_BasicType_Byte,
                            0,  // elementCount
                            "", // constraints
                            "", // defaultValue
                            ParameterSpec::ReadWriteAccess));

  ns->commands.add("flushFile", CommandSpec("Flush file data to disk"));

  ns->commands.add("closeFile",
                   CommandSpec("Close the current file, if open."));

  return ns;
}

size_t
VectorFileEffector::getNodeOutputElementCount(const std::string &outputName) const {
  NTA_THROW
      << "VectorFileEffector::getNodeOutputElementCount -- unknown output '"
      << outputName << "'";
}


bool VectorFileEffector::operator==(const RegionImpl &o) const {
  if (o.getType() != "VectorFileEffector") return false;
  VectorFileEffector& other = (VectorFileEffector&)o;
  if (filename_ != other.filename_) return false;

  return true;
}

} // namespace htm
