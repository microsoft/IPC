#include "stdafx.h"
#include "IPC/Acceptor.h"
#include "IPC/Connector.h"
#include "IPC/detail/RandomString.h"
#include "WaitHandleFactoryMock.h"
#include "TraitsMock.h"

#include <boost/test/data/test_case.hpp>


using namespace IPC;

using Traits = UnitTest::Mocks::Traits;

namespace IPC
{
    std::ostream& operator<<(std::ostream& os, const ChannelSettings<Traits>& settings)
    {
        auto getName = [](auto& memory) { return memory ? memory->GetName() : "NULL"; };

        auto flags = os.setf(std::ios_base::boolalpha);

        os  << "{ Shared=" << settings.IsSharedInputOutput()
            << " Input{ Common=" << getName(settings.GetCommonInput()) << " Override=" << settings.IsInputOverrideAllowed()
            << " } Output{ Common=" << getName(settings.GetCommonOutput()) << " Override=" << settings.IsOutputOverrideAllowed()
            << " } }";

        os.setf(flags);

        return os;
    }
} // IPC


BOOST_AUTO_TEST_SUITE(ChannelFactoryTests)

static constexpr auto c_defaultMemorySize = 1024 * 1024;

auto MakeChannelSettings(
    Traits::WaitHandleFactory waitHandleFactory,
    std::shared_ptr<SharedMemoryCache> cache,
    std::shared_ptr<SharedMemory> commonInput,
    std::shared_ptr<SharedMemory> commonOutput,
    bool allowInputOverride,
    bool allowOutputOverride)
{
    ChannelSettings<Traits> settings{ waitHandleFactory, {}, std::move(cache) };
    
    if (commonInput)
    {
        settings.SetInput(std::move(commonInput), allowInputOverride);
    }
    else
    {
        settings.SetInput(c_defaultMemorySize, allowInputOverride);
    }

    if (commonOutput)
    {
        settings.SetOutput(std::move(commonOutput), allowOutputOverride);
    }
    else
    {
        settings.SetOutput(c_defaultMemorySize, allowInputOverride);
    }

    return settings;
}

auto MakeChannelSettings(
    Traits::WaitHandleFactory waitHandleFactory,
    std::shared_ptr<SharedMemoryCache> cache,
    std::shared_ptr<SharedMemory> common,
    bool allowOverride)
{
    ChannelSettings<Traits> settings{ waitHandleFactory, {}, std::move(cache) };

    if (common)
    {
        settings.SetInputOutput(std::move(common), allowOverride);
    }
    else
    {
        settings.SetInputOutput(c_defaultMemorySize, allowOverride);
    }

    return settings;
}

auto GenerateChannelSettings(
    Traits::WaitHandleFactory waitHandleFactory,
    std::shared_ptr<SharedMemoryCache> cache,
    std::initializer_list<std::shared_ptr<SharedMemory>> memories)
{
    std::vector<ChannelSettings<Traits>> settings;

    for (auto common1 : memories)
    for (auto allowOverride1 : { true, false })
    {
        settings.push_back(MakeChannelSettings(waitHandleFactory, cache, common1, allowOverride1));

        for (auto common2 : memories)
        for (auto allowOverride2 : { true, false })
        {
            settings.push_back(MakeChannelSettings(waitHandleFactory, cache, common1, common2, allowOverride1, allowOverride2));
        }
    }

    return settings;
}

auto GenerateChannelSettings(bool acceptor)
{
    static Traits::WaitHandleFactory waitHandleFactory;
    static auto memoryCache = std::shared_ptr<SharedMemoryCache>();
    static std::shared_ptr<SharedMemory> noMemory;
    static auto connectorMemory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_defaultMemorySize);
    static auto commonMemory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_defaultMemorySize);
    static auto acceptorMemory = std::make_shared<SharedMemory>(create_only, detail::GenerateRandomString().c_str(), c_defaultMemorySize);

    auto acceptorMemories = { noMemory, acceptorMemory, commonMemory };
    auto connectorMemories = { noMemory, connectorMemory, commonMemory };

    return GenerateChannelSettings(waitHandleFactory, memoryCache, acceptor ? acceptorMemories : connectorMemories);
}

void ValidateFailure(std::vector<std::future<std::unique_ptr<IPC::Connection<int, int, Traits>>>> connections)
{
    for (auto&& futureConnection : connections)
    {
        BOOST_CHECK_THROW(futureConnection.get(), std::exception);
    }
}

void ValidateSuccess(
    const ChannelSettings<Traits>& channelSettings,
    std::vector<std::future<std::unique_ptr<IPC::Connection<int, int, Traits>>>> connections)
{
    auto&& commonInput = channelSettings.GetCommonInput();
    auto&& commonOutput = channelSettings.GetCommonOutput();

    for (auto&& futureConnection : connections)
    {
        std::unique_ptr<IPC::Connection<int, int, Traits>> connection;

        BOOST_CHECK_NO_THROW(connection = futureConnection.get());
        BOOST_TEST(!!connection);

        auto&& input = connection->GetInputChannel().GetMemory();
        auto&& output = connection->GetOutputChannel().GetMemory();

        if (!channelSettings.IsInputOverrideAllowed() && commonInput)
        {
            BOOST_TEST(commonInput == input);
        }

        if (!channelSettings.IsOutputOverrideAllowed() && commonOutput)
        {
            BOOST_TEST(commonOutput == output);
        }

        if (channelSettings.IsSharedInputOutput())
        {
            BOOST_TEST(input == output);
        }
    }
}

auto GetMemory(bool input, const ChannelSettings<Traits>& localChannelSettings, const ChannelSettings<Traits>& remoteChannelSettings)
{
    std::shared_ptr<SharedMemory> memory;

    if (const auto& common = input ? localChannelSettings.GetCommonInput() : localChannelSettings.GetCommonOutput())
    {
        memory = common;
    }

    if (input ? localChannelSettings.IsInputOverrideAllowed() : localChannelSettings.IsOutputOverrideAllowed())
    {
        if (const auto& common = input ? remoteChannelSettings.GetCommonOutput() : remoteChannelSettings.GetCommonInput())
        {
            memory = common;
        }
    }

    return memory;
}

void Validate(
    const ChannelSettings<Traits>& connectorChannelSettings,
    const ChannelSettings<Traits>& acceptorChannelSettings,
    std::vector<std::future<std::unique_ptr<IPC::Connection<int, int, Traits>>>> connectorConnections,
    std::vector<std::future<std::unique_ptr<IPC::Connection<int, int, Traits>>>> acceptorConnections)
{
    auto&& connectorInput = GetMemory(true, connectorChannelSettings, acceptorChannelSettings);
    auto&& connectorOutput = GetMemory(false, connectorChannelSettings, acceptorChannelSettings);
    auto&& acceptorInput = GetMemory(true, acceptorChannelSettings, connectorChannelSettings);
    auto&& acceptorOutput = GetMemory(false, acceptorChannelSettings, connectorChannelSettings);

    if (connectorChannelSettings.IsSharedInputOutput())
    {
        BOOST_TEST(connectorChannelSettings.GetCommonInput() == connectorChannelSettings.GetCommonOutput());
        BOOST_TEST(connectorChannelSettings.IsInputOverrideAllowed() == connectorChannelSettings.IsOutputOverrideAllowed());

        connectorInput = connectorOutput;
    }

    if (acceptorChannelSettings.IsSharedInputOutput())
    {
        BOOST_TEST(acceptorChannelSettings.GetCommonInput() == acceptorChannelSettings.GetCommonOutput());
        BOOST_TEST(acceptorChannelSettings.IsInputOverrideAllowed() == acceptorChannelSettings.IsOutputOverrideAllowed());

        acceptorOutput = acceptorInput;
    }

    BOOST_TEST(connectorConnections.size() == acceptorConnections.size());

    bool fail = (connectorInput != acceptorOutput)
        || (connectorOutput != acceptorInput)
        || ((connectorChannelSettings.IsSharedInputOutput() != acceptorChannelSettings.IsSharedInputOutput())
            && (!acceptorInput || !acceptorOutput || !connectorInput || !connectorOutput));

    if (fail)
    {
        ValidateFailure(std::move(connectorConnections));
        ValidateFailure(std::move(acceptorConnections));
    }
    else
    {
        ValidateSuccess(connectorChannelSettings, std::move(connectorConnections));
        ValidateSuccess(acceptorChannelSettings, std::move(acceptorConnections));
    }
}

BOOST_DATA_TEST_CASE(ChannelSettingsCombinationsTest,
    GenerateChannelSettings(true) * GenerateChannelSettings(false),
    acceptorChannelSettings,
    connectorChannelSettings)
{
    auto name = detail::GenerateRandomString();

    std::vector<std::future<std::unique_ptr<IPC::Connection<int, int, Traits>>>> connectorConnections, acceptorConnections;

    Acceptor<int, int, Traits> acceptor{
        name.c_str(),
        [&](auto&& futureConnection) { acceptorConnections.push_back(std::move(futureConnection)); },
        acceptorChannelSettings };

    Connector<int, int, Traits> connector{ connectorChannelSettings };

    auto waitHandleFactory = acceptorChannelSettings.GetWaitHandleFactory();

    constexpr auto ConnectionCount = 3;
    for (int i = 0; i < ConnectionCount; ++i)
    {
        connectorConnections.push_back(connector.Connect(name.c_str()));
        BOOST_TEST(waitHandleFactory.Process() != 0);
    }

    BOOST_TEST(connectorConnections.size() == ConnectionCount);

    Validate(connectorChannelSettings, acceptorChannelSettings, std::move(connectorConnections), std::move(acceptorConnections));
}

BOOST_AUTO_TEST_SUITE_END()
