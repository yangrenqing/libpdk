// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by softboy on 2018/05/11.

#include <cstdio>
#include "gtest/gtest.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/text/RegularExpression.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdktest/PdkTest.h"

#include <vector>

#define PDKTEST_DIR_SEP "/"
#define APP_FILENAME(name) Latin1String(PDKTEST_PROCESS_APPS_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name)) 

using pdk::os::process::Process;
using pdk::io::fs::TemporaryDir;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;
using pdk::io::IoDevice;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::ds::StringList;
using pdk::ds::ByteArray;
using pdk::utils::ScopedPointer;
using pdk::kernel::Object;

using ProcessFinishedSignal1 = void (Process::*)(int);
using ProcessFinishedSignal2 = void (Process::*)(int, Process::ExitStatus);
using ProcessErrorSignal = void (Process::*)(Process::ProcessError);

int sg_argc;
char **sg_argv;

class ProcessTest : public ::testing::Test
{
public:
   static void SetUpTestCase()
   {
      ASSERT_TRUE(m_temporaryDir.isValid()) << pdk_printable(m_temporaryDir.getErrorString());
      // chdir to our testdata path and execute helper apps relative to that.
      String testdataDir = FileInfo(Latin1String(PDKTEST_CURRENT_TEST_DIR)).getAbsolutePath();
      ASSERT_TRUE(Dir::setCurrent(testdataDir)) << pdk_printable(Latin1String("Could not chdir to ") + testdataDir);
   }
   
   static void TearDownTestCase()
   {
      
   }
   
private:
   static pdk::pint64 m_bytesAvailable;
   static TemporaryDir m_temporaryDir;
};

TemporaryDir ProcessTest::m_temporaryDir;
pdk::pint64 ProcessTest::m_bytesAvailable = 0;

TEST_F(ProcessTest, testGetSetCheck)
{
   Process obj1;
   // ProcessChannelMode Process::readChannelMode()
   // void Process::setReadChannelMode(ProcessChannelMode)
   obj1.setReadChannelMode(Process::ProcessChannelMode::SeparateChannels);
   ASSERT_EQ(Process::ProcessChannelMode::SeparateChannels, obj1.getReadChannelMode());
   obj1.setReadChannelMode(Process::ProcessChannelMode::MergedChannels);
   ASSERT_EQ(Process::ProcessChannelMode::MergedChannels, obj1.getReadChannelMode());
   obj1.setReadChannelMode(Process::ProcessChannelMode::ForwardedChannels);
   ASSERT_EQ(Process::ProcessChannelMode::ForwardedChannels, obj1.getReadChannelMode());
   
   // ProcessChannel Process::readChannel()
   // void Process::setReadChannel(ProcessChannel)
   obj1.setReadChannel(Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(Process::ProcessChannel::StandardOutput, obj1.getReadChannel());
   obj1.setReadChannel(Process::ProcessChannel::StandardError);
   ASSERT_EQ(Process::ProcessChannel::StandardError, obj1.getReadChannel());
}

TEST_F(ProcessTest, testConstructing)
{
   Process process;
   ASSERT_EQ(process.getReadChannel(), Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(process.getWorkingDirectory(), String());
   ASSERT_EQ(process.getProcessEnvironment().toStringList(), StringList());
   ASSERT_EQ(process.getError(), Process::ProcessError::UnknownError);
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   ASSERT_EQ(process.getProcessId(), PDK_PID(0));
   ASSERT_EQ(process.readAllStandardOutput(), ByteArray());
   ASSERT_EQ(process.readAllStandardError(), ByteArray());
   ASSERT_EQ(process.canReadLine(), false);
   
   // IoDevice
   ASSERT_EQ(process.getOpenMode(), IoDevice::OpenMode::NotOpen);
   ASSERT_TRUE(!process.isOpen());
   ASSERT_TRUE(!process.isReadable());
   ASSERT_TRUE(!process.isWritable());
   ASSERT_TRUE(process.isSequential());
   ASSERT_EQ(process.getPosition(), pdk::plonglong(0));
   ASSERT_EQ(process.getSize(), pdk::plonglong(0));
   ASSERT_TRUE(process.atEnd());
   ASSERT_EQ(process.getBytesAvailable(), pdk::plonglong(0));
   ASSERT_EQ(process.getBytesToWrite(), pdk::plonglong(0));
   ASSERT_TRUE(!process.getErrorString().isEmpty());
   
   char c;
   ASSERT_EQ(process.read(&c, 1), pdk::plonglong(-1));
   ASSERT_EQ(process.write(&c, 1), pdk::plonglong(-1));
}

TEST_F(ProcessTest, testSimpleStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ScopedPointer<Process> process(new Process);
   process->connectReadyReadSignal([](IoDevice::SignalType, Object *sender){
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      int lines = 0;
      while (process->canReadLine()) {
         ++lines;
         process->readLine();
      }
   }, process.getData());
   std::list<Process::ProcessState> stateChangedData;
   process->connectStateChangedSignal([&stateChangedData](Process::ProcessState state){
      stateChangedData.push_back(state);
   }, PDK_RETRIEVE_APP_INSTANCE());
   process->start(APP_FILENAME(ProcessNormalApp));
   
   if(process->getState() != Process::ProcessState::Starting) {
      ASSERT_EQ(process->getState(), Process::ProcessState::Running);
   }
   
   ASSERT_TRUE(process->waitForStarted(5000)) << pdk_printable(process->getErrorString());
   ASSERT_EQ(process->getState(), Process::ProcessState::Running);
   PDK_TRY_COMPARE(process->getState(), Process::ProcessState::NotRunning);
   process.reset();
   ASSERT_EQ(stateChangedData.size(), 3u);
   auto iter = stateChangedData.begin();
   ASSERT_EQ(*iter++, Process::ProcessState::Starting);
   ASSERT_EQ(*iter++, Process::ProcessState::Running);
   ASSERT_EQ(*iter++, Process::ProcessState::NotRunning);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartWithOpen)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   ASSERT_EQ(process.open(IoDevice::OpenMode::ReadOnly), false);
   process.setProgram(APP_FILENAME(ProcessNormalApp));
   ASSERT_EQ(process.getProgram(), APP_FILENAME(ProcessNormalApp));
   process.setArguments(StringList() << Latin1String("arg1") << Latin1String("arg2"));
   ASSERT_EQ(process.getArguments().size(), 2u);
   ASSERT_TRUE(process.open(IoDevice::OpenMode::ReadOnly));
   ASSERT_EQ(process.getOpenMode(), IoDevice::OpenMode::ReadOnly);
   ASSERT_TRUE(process.waitForFinished(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartWithOldOpen)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   class OverriddenOpen : public Process
   {
   public:
      virtual bool open(OpenModes mode) override
      { return IoDevice::open(mode); }
   };
   OverriddenOpen process;
   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(process.waitForFinished(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testExecute)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ASSERT_EQ(Process::execute(APP_FILENAME(ProcessNormalApp),
                              StringList() << Latin1String("arg1") << Latin1String("arg2")), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartDetached)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ASSERT_TRUE(Process::startDetached(APP_FILENAME(ProcessNormalApp),
                                      StringList() << Latin1String("arg1") << Latin1String("arg2")));
   ASSERT_EQ(Process::startDetached(Latin1String("nonexistingexe")), false);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCrashTest)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ScopedPointer<Process> process(new Process);
   
   std::list<Process::ProcessState> stateChangedData;
   process->connectStateChangedSignal([&stateChangedData](Process::ProcessState state){
      stateChangedData.push_back(state);
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   process->start(APP_FILENAME(ProcessCrashApp));
   
   ASSERT_TRUE(process->waitForStarted(5000));
   
   std::list<Process::ProcessError> errorData;
   process->connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   std::list<Process::ExitStatus> exitStatusData;
   process->connectFinishedSignal([&exitStatusData](int exitCode, Process::ExitStatus status){
      exitStatusData.push_back(status);
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   ASSERT_TRUE(process->waitForFinished(30000));
   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);
   ASSERT_EQ(exitStatusData.size(), 1u);
   ASSERT_EQ(*exitStatusData.begin(), Process::ExitStatus::CrashExit);
   process.reset();
   
   ASSERT_EQ(stateChangedData.size(), 3u);
   auto iter = stateChangedData.begin();
   ASSERT_EQ(*iter++, Process::ProcessState::Starting);
   ASSERT_EQ(*iter++, Process::ProcessState::Running);
   ASSERT_EQ(*iter++, Process::ProcessState::NotRunning);
   
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCrashTest2)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessCrashApp));
   ASSERT_TRUE(process.waitForStarted(5000));
   
   std::list<Process::ProcessError> errorData;
   process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   std::list<Process::ExitStatus> exitStatusData;
   process.connectFinishedSignal([&exitStatusData](int exitCode, Process::ExitStatus status){
      exitStatusData.push_back(status);
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   process.connectFinishedSignal([](int exitCode, Process::ExitStatus status){
      pdktest::TestEventLoop::instance().exitLoop();
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   pdktest::TestEventLoop::instance().enterLoop(30);
   if (pdktest::TestEventLoop::instance().getTimeout()) {
      FAIL() << "Failed to detect crash : operation timed out";
   }
   
   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);
   ASSERT_EQ(exitStatusData.size(), 1u);
   ASSERT_EQ(*exitStatusData.begin(), Process::ExitStatus::CrashExit);
   
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::CrashExit);
   
   PDKTEST_END_APP_CONTEXT();
}

namespace {

using ExitStatusDataType = std::list<std::tuple<StringList, std::vector<Process::ExitStatus>>>;
void init_exit_status_data(ExitStatusDataType &data)
{
   {
      StringList strList;
      strList << APP_FILENAME(ProcessNormalApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }
   
   {
      StringList strList;
      strList << APP_FILENAME(ProcessCrashApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }
   
   {
      StringList strList;
      strList << APP_FILENAME(ProcessNormalApp)
              << APP_FILENAME(ProcessCrashApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }
   
   {
      StringList strList;
      strList << APP_FILENAME(ProcessCrashApp)
              << APP_FILENAME(ProcessNormalApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }
}

} // anonymous namespace

TEST_F(ProcessTest, testExitStatus)
{
   ExitStatusDataType data;
   init_exit_status_data(data);
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   for (auto &item : data) {
      StringList &processList = std::get<0>(item);
      std::vector<Process::ExitStatus> exitStatus = std::get<1>(item);
      ASSERT_EQ(exitStatus.size(), processList.size());
      for (size_t i = 0; i < processList.size(); ++i) {
         process.start(processList.at(i));
         ASSERT_TRUE(process.waitForStarted(5000));
         ASSERT_TRUE(process.waitForFinished(30000));
         
         ASSERT_EQ(process.getExitStatus(), exitStatus.at(i));
      }
   }
   
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testLoopBack)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   for (int i = 0; i < 100; ++i) {
      process.write("Hello");
      do {
         ASSERT_TRUE(process.waitForReadyRead(5000));
      } while (process.getBytesAvailable() < 5);
      ASSERT_EQ(process.readAll(), ByteArray("Hello"));
   }
   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testReadTimeoutAndThenCrash)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   if (process.getState() != Process::ProcessState::Starting) {
      ASSERT_EQ(process.getState(), Process::ProcessState::Running);
   }

   std::list<Process::ProcessError> errorData;
   process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());

   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_EQ(process.getState(), Process::ProcessState::Running);

   ASSERT_TRUE(!process.waitForReadyRead(5000));
   ASSERT_EQ(process.getError(), Process::ProcessError::Timedout);

   process.kill();

   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);

   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   
   Process process;
   process.start(APP_FILENAME(ProcessOutputApp));
   
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   
   String output = Latin1String(process.readAll());
   ASSERT_EQ(output.count(Latin1String("\n")), 10 * 1024);
   
   process.start(Latin1String("notexitloop"));
   
   ASSERT_TRUE(!process.waitForFinished());
   ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
   
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testRestartProcessDeadlock)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   
   Process process;
   auto conn = process.connectFinishedSignal([](int exitCode, Process::ExitStatus status, Process::SignalType signal, Object *sender){
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      process->start(APP_FILENAME(ProcessEchoApp));
   }, PDK_RETRIEVE_APP_INSTANCE());
   
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_EQ(process.write("", 1), pdk::plonglong(1));
   ASSERT_TRUE(process.waitForFinished(5000));
   process.disconnectFinishedSignal(conn);
   
   ASSERT_EQ(process.write("", 1), pdk::plonglong(1));
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCloseWriteChannel)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ByteArray testData("Data to read");
   Process more;
   more.start(APP_FILENAME(ProcessEOFApp));
   
   ASSERT_TRUE(more.waitForStarted(5000));
   ASSERT_TRUE(!more.waitForReadyRead(250));
   ASSERT_EQ(more.getError(), Process::ProcessError::Timedout);
   
   ASSERT_EQ(more.write(testData), pdk::pint64(testData.size()));
   
   ASSERT_TRUE(!more.waitForReadyRead(250));
   ASSERT_EQ(more.getError(), Process::ProcessError::Timedout);
   
   more.closeWriteChannel();
   
   while (more.getBytesAvailable() < testData.size()) {
      ASSERT_TRUE(more.waitForReadyRead(5000));
   }
   
   ASSERT_EQ(more.readAll(), testData);
   
   if (more.getState() == Process::ProcessState::Running) {
      ASSERT_TRUE(more.waitForFinished(5000));
   }
   
   ASSERT_EQ(more.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(more.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

int main(int argc, char **argv)
{
   sg_argc = argc;
   sg_argv = argv;
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}