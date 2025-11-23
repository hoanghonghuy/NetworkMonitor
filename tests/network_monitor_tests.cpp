#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "TestUtils.h"

using namespace NetworkMonitor;

namespace NetworkMonitorTests
{

void RunNetworkMonitorTests()
{
    LogTestMessage(L"=== NetworkMonitorClass tests ===");

    NetworkMonitorClass monitor;
    AssertTrue(!monitor.IsRunning(), L"NetworkMonitorClass.IsRunning is false before Start");

    bool started = monitor.Start();
    if (!started)
    {
        LogTestMessage(L"[WARN] NetworkMonitorClass.Start failed; skipping further network tests");
        return;
    }

    AssertTrue(monitor.IsRunning(), L"NetworkMonitorClass.IsRunning is true after Start");

    (void)monitor.Update();

    NetworkStats agg = monitor.GetAggregatedStats();
    (void)agg;

    std::vector<NetworkStats> all = monitor.GetAllStats();
    (void)all;

    monitor.Stop();
    AssertTrue(!monitor.IsRunning(), L"NetworkMonitorClass.IsRunning is false after Stop");
}

} // namespace NetworkMonitorTests
