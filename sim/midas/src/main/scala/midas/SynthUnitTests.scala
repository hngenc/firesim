// See LICENSE for license details.

package midas.unittest

import midas.core._

import chisel3._
import firrtl.{ExecutionOptionsManager, HasFirrtlOptions}

import freechips.rocketchip.config.{Parameters, Config, Field}
import freechips.rocketchip.unittest.{UnitTests, TestHarness}
import midas.models.{CounterTableUnitTest, LatencyHistogramUnitTest, AddressRangeCounterUnitTest}


// Unittests
class WithAllUnitTests extends Config((site, here, up) => {
  case UnitTests => (q: Parameters) => {
    implicit val p = q
    val timeout = 2000000
    Seq(
      Module(new PipeChannelUnitTest(latency = 0, timeout = timeout)),
      Module(new PipeChannelUnitTest(latency = 1, timeout = timeout)),
      Module(new ReadyValidChannelUnitTest(timeout = timeout)),
      Module(new CounterTableUnitTest),
      Module(new LatencyHistogramUnitTest),
      Module(new AddressRangeCounterUnitTest)
    )
  }
})

// Failing tests
class WithTimeOutCheck extends Config((site, here, up) => {
  case UnitTests => (q: Parameters) => {
    implicit val p = q
    Seq(
      Module(new PipeChannelUnitTest(timeout = 100)),
    )
  }
})

// Timestamped model tests
class WithTimestampTests extends Config((site, here, up) => {
  case UnitTests => (q: Parameters) => {
    implicit val p = q
    import midas.widgets._
    Seq(
      //Module(new ClockSourceTest(1000, true)),
      Module(new TimestampedRegisterTest(Posedge)),
    )
  }
})

// Complete configs
class AllUnitTests extends Config(new WithAllUnitTests ++ new midas.SimConfig)
class TimeOutCheck extends Config(new WithTimeOutCheck ++ new midas.SimConfig)
class TimestampTests extends Config(new WithTimestampTests ++ new midas.SimConfig)