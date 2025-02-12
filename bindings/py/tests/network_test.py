# ----------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2017, Numenta, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
# ----------------------------------------------------------------------

import json
import unittest
import pytest


from htm.bindings.regions.PyRegion import PyRegion

import htm.bindings.engine_internal as engine
from htm.bindings.tools.serialization_test_py_region import \
     SerializationTestPyRegion


class LinkRegion(PyRegion):
  """
  Test region used to test link validation
  """
  def __init__(self): pass
  def initialize(self): pass
  def compute(self): pass
  def getOutputElementCount(self, name): 
    return 1
  @classmethod
  def getSpec(cls):
    return {
      "description": LinkRegion.__doc__,
      "singleNodeOnly": True,
      "inputs": {
        "UInt32": {
          "description": "UInt32 Data",
          "dataType": "UInt32",
          "isDefaultInput": True,
          "required": False,
          "count": 0
        },
        "Real32": {
          "description": "Real32 Data",
          "dataType": "Real32",
          "isDefaultInput": False,
          "required": False,
          "count": 0
        },
      },
      "outputs": {
        "UInt32": {
          "description": "UInt32 Data",
          "dataType": "UInt32",
          "isDefaultOutput": True,
          "required": False,
          "count": 0
        },
        "Real32": {
          "description": "Real32 Data",
          "dataType": "Real32",
          "isDefaultOutput": False,
          "required": False,
          "count": 0
        },
      },
      "parameters": { }
    }

class NetworkTest(unittest.TestCase):

  def setUp(self):
    """Register test region"""
    engine.Network.cleanup()
    engine.Network.registerPyRegion(LinkRegion.__module__, LinkRegion.__name__)

  @pytest.mark.skip(reason="pickle support needs work...another PR")
  def testSerializationWithPyRegion(self):
    """Test  (de)serialization of network containing a python region"""
    engine.Network.registerPyRegion(__name__,
                                    SerializationTestPyRegion.__name__)
    try:
      srcNet = engine.Network()
      srcNet.addRegion(SerializationTestPyRegion.__name__,
                       "py." + SerializationTestPyRegion.__name__,
                       json.dumps({
                         "dataWidth": 128,
                         "randomSeed": 99,
                       }))

      # Serialize
      srcNet.saveToFile("SerializationTest.stream")


      # Deserialize
      destNet = engine.Network()
      destNet.loadFromFile("SerializationTest.stream")

      destRegion = destNet.getRegion(SerializationTestPyRegion.__name__)

      self.assertEqual(destRegion.getParameterUInt32("dataWidth"), 128)
      self.assertEqual(destRegion.getParameterUInt32("randomSeed"), 99)

    finally:
      engine.Network.unregisterPyRegion(SerializationTestPyRegion.__name__)


  def testSimpleTwoRegionNetworkIntrospection(self):
    # Create Network instance
    network = engine.Network()

    # Add two TestNode regions to network
    network.addRegion("region1", "TestNode", "")
    network.addRegion("region2", "TestNode", "")

    # Set dimensions on first region
    region1 = network.getRegion("region1")
    region1.setDimensions(engine.Dimensions([1, 1]))

    # Link region1 and region2
    network.link("region1", "region2")

    # Initialize network
    network.initialize()

    for link in network.getLinks():
      # Compare Link API to what we know about the network
      self.assertEqual(link.getDestRegionName(), "region2")
      self.assertEqual(link.getSrcRegionName(), "region1")
      self.assertEqual(link.getDestInputName(), "bottomUpIn")
      self.assertEqual(link.getSrcOutputName(), "bottomUpOut")
      break
    else:
      self.fail("Unable to iterate network links.")


  def testNetworkLinkTypeValidation(self):
    """
    This tests whether the links source and destination dtypes match
    """
    network = engine.Network()
    r_from = network.addRegion("from", "py.LinkRegion", "")
    r_to = network.addRegion("to", "py.LinkRegion", "")
    cnt = r_from.getOutputElementCount("UInt32")
    self.assertEqual(1, cnt)


    # Check for valid links
    network.link("from", "to", "", "", "UInt32", "UInt32")
    network.link("from", "to", "", "", "Real32", "Real32")
    network.link("from", "to", "", "", "Real32", "UInt32")
    network.link("from", "to", "", "", "UInt32", "Real32")
	

  @pytest.mark.skip(reason="parameter types don't match.")
  def testParameters(self):

    n = engine.Network()
    l1 = n.addRegion("l1", "TestNode", "")
    scalars = [
      ("int32Param", l1.getParameterInt32, l1.setParameterInt32, 32, int, 35),
      ("uint32Param", l1.getParameterUInt32, l1.setParameterUInt32, 33, int, 36),
      ("int64Param", l1.getParameterInt64, l1.setParameterInt64, 64, int, 74),
      ("uint64Param", l1.getParameterUInt64, l1.setParameterUInt64, 65, int, 75),
      ("real32Param", l1.getParameterReal32, l1.setParameterReal32, 32.1, float, 33.1),
      ("real64Param", l1.getParameterReal64, l1.setParameterReal64, 64.1, float, 65.1),
      ("stringParam", l1.getParameterString, l1.setParameterString, "nodespec value", str, "new value")]

    for paramName, paramGetFunc, paramSetFunc, initval, paramtype, newval in scalars:
      # Check the initial value for each parameter.
      x = paramGetFunc(paramName)
      self.assertEqual(type(x), paramtype, paramName)
      if initval is None:
        continue
      if type(x) == float:
        self.assertTrue(abs(x - initval) < 0.00001, paramName)
      else:
        self.assertEqual(x, initval, paramName)

      # Now set the value, and check to make sure the value is updated
      paramSetFunc(paramName, newval)
      x = paramGetFunc(paramName)
      self.assertEqual(type(x), paramtype)
      if type(x) == float:
        self.assertTrue(abs(x - newval) < 0.00001)
      else:
        self.assertEqual(x, newval)
