/*
    KStars UI tests for capture workflows (re-focus, dithering, guiding, ...)

    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "test_ekos_capture_workflow.h"

#include "kstars_ui_tests.h"
#include "ui_calibrationoptions.h"
#include "indicom.h"
#include "indi/guimanager.h"
#include "Options.h"
#include "skymapcomposite.h"
#include "ekos/capture/sequencejobstate.h"

#define SHUTTER_UNKNOWN -1
#define SHUTTER_NO       0
#define SHUTTER_YES      1


TestEkosCaptureWorkflow::TestEkosCaptureWorkflow(QObject *parent) :
    TestEkosCaptureWorkflow::TestEkosCaptureWorkflow("Internal", parent) {}

TestEkosCaptureWorkflow::TestEkosCaptureWorkflow(QString guider, QObject *parent) : QObject(parent)
{
    m_CaptureHelper = new TestEkosCaptureHelper(guider);
    m_CaptureHelper->m_GuiderDevice = "CCD Simulator";
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
}

void TestEkosCaptureWorkflow::testCaptureRefocusDelay()
{
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    Ekos::Manager *manager = Ekos::Manager::Instance();
    QVERIFY(prepareCapture(1));
    QVERIFY(m_CaptureHelper->executeFocusing());

    // start capturing, expect focus after first captured frame
    Ekos::Capture *capture = manager->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_FOCUSING);
    KTRY_CLICK(capture, startB);
    // focusing must have started 60 secs + exposure time + 10 secs delay
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates,
                                     60000 + 10000 + 1000 * capture->captureExposureN->value());
}

void TestEkosCaptureWorkflow::testCaptureRefocusHFR()
{
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    Ekos::Manager *manager = Ekos::Manager::Instance();
    QVERIFY(prepareCapture(0, 1.2));
    QVERIFY(m_CaptureHelper->executeFocusing());

    // start capturing
    Ekos::Capture *capture = manager->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    m_CaptureHelper->expectedFocusStates.append(Ekos::FOCUS_PROGRESS);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    KTRY_CLICK(capture, startB);
    // wait for one frame has been captured:   exposure time + 10 secs delay
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates,
                                     10000 +  1000 * capture->captureExposureN->value());
    // now move the focuser twice to increase the HFR
    KTRY_CLICK(manager->focusModule(), focusOutB);
    KTRY_CLICK(manager->focusModule(), focusOutB);
    // check if focusing has started, latest after two more frames
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedFocusStates,
                                     10000 + 2 * 1000 * capture->captureExposureN->value());
}


void TestEkosCaptureWorkflow::testCaptureRefocusTemperature()
{
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    Ekos::Manager *manager = Ekos::Manager::Instance();
    Ekos::Capture *capture = manager->captureModule();
    // select temperature threshold
    double deltaT = 2.0;
    QVERIFY(prepareCapture(0, 0, deltaT));
    // set the focuser temperature
    SET_INDI_VALUE_DOUBLE(m_CaptureHelper->m_FocuserDevice, "FOCUS_TEMPERATURE", "TEMPERATURE", 0);
    // select the focuser as temperature source
    KTRY_SET_COMBO(manager->focusModule(), defaultFocusTemperatureSource, m_CaptureHelper->m_FocuserDevice);

    QVERIFY(m_CaptureHelper->executeFocusing());

    // start capturing
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    m_CaptureHelper->expectedFocusStates.append(Ekos::FOCUS_PROGRESS);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    KTRY_CLICK(capture, startB);
    // wait for one frame has been captured:   exposure time + 10 secs delay
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates,
                                     10000 +  1000 * capture->captureExposureN->value());
    // now change the temperature on the focuser
    SET_INDI_VALUE_DOUBLE(m_CaptureHelper->m_FocuserDevice, "FOCUS_TEMPERATURE", "TEMPERATURE", -2 * deltaT);

    KTRY_CLICK(manager->focusModule(), focusOutB);
    KTRY_CLICK(manager->focusModule(), focusOutB);
    // check if focusing has started, latest after two more frames
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedFocusStates,
                                     10000 + 2 * 1000 * capture->captureExposureN->value());

}


void TestEkosCaptureWorkflow::testCaptureRefocusAbort()
{
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    Ekos::Manager *manager = Ekos::Manager::Instance();
    QVERIFY(prepareCapture(1));
    QVERIFY(m_CaptureHelper->executeFocusing());

    // start capturing, expect focus after first captured frame
    Ekos::Capture *capture = manager->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    m_CaptureHelper->expectedFocusStates.append(Ekos::FOCUS_PROGRESS);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_FOCUSING);
    KTRY_CLICK(capture, startB);
    // focusing must have started 60 secs + exposure time + 10 secs delay
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedFocusStates,
                                     60000 + 10000 + 1000 * capture->captureExposureN->value());
    // the capture module must change to focusing subsequently
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 5000);
    // now abort capturing
    m_CaptureHelper->expectedFocusStates.append(Ekos::FOCUS_ABORTED);
    capture->abort();
    // expect that focusing aborts
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedFocusStates, 10000);
}

void TestEkosCaptureWorkflow::testCaptureScriptsExecution()
{
    // default initialization
    QVERIFY(prepareTestCase());

    QFETCH(bool, pausing);
    // static test with three exposures
    int count = 3;
    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // add target to path to emulate the behavior of the scheduler
    QString imagepath = getImageLocation()->path() + "/test";

    // create executable scripts
    m_CaptureHelper->createAllCaptureScripts(destination);

    // setup scripts - starts as thread since clicking on capture blocks
    bool success = false;
    QTimer::singleShot(1000, capture, [&] {success = m_CaptureHelper->fillScriptManagerDialog(m_CaptureHelper->getScripts());});
    // open script manager
    KTRY_CLICK(capture, scriptManagerB);
    // verify if script configuration succeeded
    QVERIFY2(success, "Scripts set up failed!");

    // create capture sequences
    KTRY_CAPTURE_CONFIGURE_LIGHT(2.0, count, 0.0, "Luminance", imagepath);
    KTRY_CAPTURE_CLICK(addToQueueB);
    KTRY_CAPTURE_CONFIGURE_LIGHT(2.0, count, 0.0, "Red", imagepath);
    KTRY_CAPTURE_CLICK(addToQueueB);
    // check if row has been added
    KTRY_CAPTURE_GADGET(QTableWidget, queueTable);
    QTRY_VERIFY_WITH_TIMEOUT(queueTable->rowCount() == 2, 1000);
    // wait to pause when second frame is running
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    // start capture
    KTRY_CLICK(capture, startB);
    if (pausing)
    {
        // wait for first frame to be completed
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 15000);
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_PAUSED);
        QTest::qWait(1500);
        // press pause
        KTRY_CLICK(capture, pauseB);
        // wait until paused
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 15000);
        // prepare next stage
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_PAUSED);
        QTest::qWait(3000);
        // press continue
        KTRY_CLICK(capture, startB);
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.size() <= 1, 15000);
        QTest::qWait(500);
        // press pause
        KTRY_CLICK(capture, pauseB);
        // wait until paused
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 15000);
        // press continue
        QTest::qWait(3000);
        KTRY_CLICK(capture, startB);
    }
    // wait until completed
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    // wait that all frames have been taken
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 30000);

    // check the log file if it holds the expected number
    QVERIFY(m_CaptureHelper->checkScriptRuns(count, 2));
}

void TestEkosCaptureWorkflow::testGuidingDeviationSuspendingCapture()
{
    // default initialization
    QVERIFY(prepareTestCase());

    const double deviation_limit = 2.0;
    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    // set guide deviation guard to < 2"
    KTRY_SET_CHECKBOX(capture, limitGuideDeviationS, true);
    KTRY_SET_DOUBLESPINBOX(capture, limitGuideDeviationN, deviation_limit);

    // add target to path to emulate the behavior of the scheduler
    QString imagepath = getImageLocation()->path() + "/test";
    // build a LRGB sequence
    KTRY_CAPTURE_ADD_LIGHT(30.0, 1, 5.0, "Luminance", imagepath);
    KTRY_CAPTURE_ADD_LIGHT(30.0, 1, 5.0, "Red", imagepath);
    KTRY_CAPTURE_ADD_LIGHT(30.0, 1, 5.0, "Green", imagepath);
    KTRY_CAPTURE_ADD_LIGHT(30.0, 1, 5.0, "Blue", imagepath);

    // set a position in the west
    SkyPoint *target = new SkyPoint();
    target->setAz(270.0);
    target->setAlt(KStarsData::Instance()->geo()->lat()->Degrees() / 2.0);
    // translate to equatorial coordinates
    const dms lst = KStarsData::Instance()->geo()->GSTtoLST(KStarsData::Instance()->clock()->utc().gst());
    const dms *lat = KStarsData::Instance()->geo()->lat();
    target->HorizontalToEquatorial(&lst, lat);

    m_CaptureHelper->slewTo(target->ra().Hours(), target->dec().Degrees(), true);

    // clear calibration to ensure proper guiding
    KTRY_CLICK(Ekos::Manager::Instance()->guideModule(), clearCalibrationB);

    // start guiding
    m_CaptureHelper->startGuiding(2.0);

    // start capture
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    KTRY_CLICK(capture, startB);
    // wait until capturing starts
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 10000);
    // wait for settling
    QTest::qWait(2000);
    // create a guide drift
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_SUSPENDED);
    Ekos::Manager::Instance()->mountModule()->doPulse(RA_INC_DIR, 2000, DEC_INC_DIR, 2000);
    qCInfo(KSTARS_EKOS_TEST()) << "Sent 2000ms RA+DEC guiding pulses.";
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 30000);
    // expect that capturing continues
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getCaptureStatus() == Ekos::CAPTURE_CAPTURING, 60000);
    // verify that capture starts only once
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_PROGRESS);
    QTest::qWait(20000);
    QVERIFY2(m_CaptureHelper->expectedCaptureStates.size() > 0, "Multiple capture starts.");
}

void TestEkosCaptureWorkflow::testGuidingDeviationAbortCapture()
{
    // default initialization
    QVERIFY(prepareTestCase());

    const double deviation_limit = 2.0;
    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    // set guide deviation guard to < 2"
    KTRY_SET_CHECKBOX(capture, limitGuideDeviationS, true);
    KTRY_SET_DOUBLESPINBOX(capture, limitGuideDeviationN, deviation_limit);

    // add target to path to emulate the behavior of the scheduler
    QString imagepath = getImageLocation()->path() + "/test";
    // build a simple 5xL sequence
    KTRY_CAPTURE_ADD_LIGHT(45.0, 5, 5.0, "Luminance", imagepath);
    // set Dubhe as target and slew there
    SkyObject *target = KStars::Instance()->data()->skyComposite()->findByName("Dubhe");
    m_CaptureHelper->slewTo(target->ra().Hours(), target->dec().Degrees(), true);

    // clear calibration to ensure proper guiding
    KTRY_CLICK(Ekos::Manager::Instance()->guideModule(), clearCalibrationB);

    // start guiding
    m_CaptureHelper->startGuiding(2.0);

    // start capture
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    KTRY_CLICK(capture, startB);
    // wait until capturing starts
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 10000);
    // wait for settling
    QTest::qWait(2000);
    // create a guide drift
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_SUSPENDED);
    Ekos::Manager::Instance()->mountModule()->doPulse(RA_INC_DIR, 2000, DEC_INC_DIR, 2000);
    qCInfo(KSTARS_EKOS_TEST()) << "Sent 2000ms RA+DEC guiding pulses.";
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    // wait that capturing gets suspended
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 15000);
    // abort capturing
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_ABORTED);
    KTRY_CLICK(capture, startB);
    // check that it has been aborted
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 10000);
    // wait that the guiding deviation is below the limit and
    // verify that capture does not start
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_PROGRESS);
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getGuideDeviation() < deviation_limit, 60000);

    QTest::qWait(20000);
    QVERIFY2(m_CaptureHelper->expectedCaptureStates.size() > 0, "Capture has been restarted although aborted.");
}

void TestEkosCaptureWorkflow::testInitialGuidingLimitCapture()
{
    // default initialization
    QVERIFY(prepareTestCase());

    const double deviation_limit = 2.0;
    QFETCH(double, exptime);
    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
    // set start guide deviation guard to < 2" but disable the other one
    KTRY_SET_CHECKBOX(capture, startGuiderDriftS, true);
    KTRY_SET_DOUBLESPINBOX(capture, startGuiderDriftN, deviation_limit);
    // create sequence with 10 sec delay
    QVERIFY(prepareCapture(0, 0, 0, 10));
    // set Dubhe as target and slew there
    SkyObject *target = KStars::Instance()->data()->skyComposite()->findByName("Dubhe");
    m_CaptureHelper->slewTo(target->ra().Hours(), target->dec().Degrees(), true);

    // start guiding
    m_CaptureHelper->startGuiding(2.0);

    for (int i = 1; i <= 2; i++)
    {
        // wait intially 5 seconds
        if (i == 1)
            QTest::qWait(5000);

        // prepare to expect that capturing will start
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);

        // ensure that guiding is running
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getGuidingStatus() == Ekos::GUIDE_GUIDING, 10000);

        // create a guide drift
        Ekos::Manager::Instance()->mountModule()->doPulse(RA_INC_DIR, 2000, DEC_INC_DIR, 2000);
        qCInfo(KSTARS_EKOS_TEST()) << "Sent 2000ms RA+DEC guiding pulses.";

        // wait until guide deviation is present
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getGuideDeviation() > deviation_limit, 15000);

        if (i == 1)
        {
            // start capture but expect it being suspended first
            KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);
            KTRY_CLICK(capture, startB);
        }
        // verify that capturing does not start before the guide deviation is below the limit
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getGuideDeviation() >= deviation_limit, 60000);
        // wait 3 seconds and then ensure that capture did not start
        QTest::qWait(3000);
        QTRY_VERIFY(m_CaptureHelper->expectedCaptureStates.size() > 0);
        // wait until guiding deviation is below the limit
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->getGuideDeviation() < deviation_limit, 60000);
        // wait until capturing starts
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 30000);
        if (i < 2)
        {
            // in the first iteration wait until the capture completes
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
            KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 1500 * exptime);
        }
    }
}

void TestEkosCaptureWorkflow::testCaptureWaitingForTemperature()
{
    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // initial and target temperature
    QFETCH(double, initTemp);
    QFETCH(double, targetTemp);

    // initialize the CCD temperature and wait until it is reached
    SET_INDI_VALUE_DOUBLE(m_CaptureHelper->m_CCDDevice, "CCD_TEMPERATURE", "CCD_TEMPERATURE_VALUE", initTemp);
    QTRY_VERIFY_WITH_TIMEOUT(std::abs(capture->temperatureOUT->text().toDouble()) <= Options::maxTemperatureDiff(), 60000);

    // set target temperature
    KTRY_SET_DOUBLESPINBOX(capture, cameraTemperatureN, targetTemp);
    KTRY_SET_CHECKBOX(capture, cameraTemperatureS, true);

    // build a simple 1xL sequence
    KTRY_CAPTURE_ADD_LIGHT(10.0, 5, 5.0, "Luminance", getImageLocation()->path() + "/test");
    // expect capturing state
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);

    // start capturing
    KTRY_CLICK(capture, startB);
    // check if capturing has started
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
    // check if the temperature is at the expected level
    QTRY_VERIFY2(std::abs(capture->temperatureOUT->text().toDouble() - targetTemp) <= Options::maxTemperatureDiff(),
                 QString("Temperature %1°C not at the expected level of %2°C").arg(capture->temperatureOUT->text()).arg(
                     targetTemp).toLocal8Bit());

    // stop capturing
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_ABORTED);
    KTRY_CLICK(capture, startB);
    // check if capturing has stopped
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 10000);

    // restart again to check whether an already reached temperature is handled properly
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    KTRY_CLICK(capture, startB);
    // check if capturing has started
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 20000);
    // check if the temperature is at the expected level
    QTRY_VERIFY2(std::abs(capture->temperatureOUT->text().toDouble() - targetTemp) <= Options::maxTemperatureDiff(),
                 QString("Temperature %1°C not at the expected level of %2°C").arg(capture->temperatureOUT->text()).arg(
                     targetTemp).toLocal8Bit());

    // stop capturing
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_ABORTED);
    KTRY_CLICK(capture, startB);
    // check if capturing has stopped
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 10000);

    // change temperature back to initial value
    SET_INDI_VALUE_DOUBLE(m_CaptureHelper->m_CCDDevice, "CCD_TEMPERATURE", "CCD_TEMPERATURE_VALUE", initTemp);
    QTRY_VERIFY_WITH_TIMEOUT(std::abs(capture->temperatureOUT->text().toDouble()) <= Options::maxTemperatureDiff(), 60000);

    // start capturing for a second time
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    KTRY_CLICK(capture, startB);
    // check if capturing has started
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
    // check if the temperature is at the expected level
    QTRY_VERIFY2(std::abs(capture->temperatureOUT->text().toDouble() - targetTemp) <= Options::maxTemperatureDiff(),
                 QString("Temperature %1°C not at the expected level of %2°C").arg(capture->temperatureOUT->text()).arg(
                     targetTemp).toLocal8Bit());

}

void TestEkosCaptureWorkflow::testCaptureWaitingForRotator()
{
    // use the rotator simulator
    m_CaptureHelper->m_RotatorDevice = "Rotator Simulator";

    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // open the rotator dialog
    KTRY_CLICK(capture, rotatorB);

    // ensure that the rotator dialog appears
    QWidget *rotatorDialog = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((rotatorDialog = Ekos::Manager::Instance()->findChild<QWidget *>("RotatorDialog")) != nullptr,
                             10000);

    // target angle rotation
    double targetAngle = 90.0;
    // set the target rotation angle
    KTRY_SET_DOUBLESPINBOX(rotatorDialog, CameraPA, targetAngle);
    // save it to the sequence job
    KTRY_SET_CHECKBOX(rotatorDialog, enforceJobPA, true);

    // build a simple 1xL sequence
    KTRY_CAPTURE_ADD_LIGHT(30.0, 1, 5.0, "Luminance", getImageLocation()->path() + "/test");
    // expect capturing state
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);

    // set the rotation angle back
    CameraPA->setValue(0);
    // let the rotator rotate back for at least 3sec
    QTest::qWait(3000);

    // start capturing
    KTRY_CLICK(capture, startB);
    // check if capturing has started
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);

    // KTRY_GADGET(rotatorDialog, QLineEdit, rawAngleOut);
    // QTRY_VERIFY2(fabs(rawAngleOut->text().toDouble() - targetAngle) * 60 <= Options::astrometryRotatorThreshold(),
    //              QString("Rotator angle %1° not at the expected value of %2°").arg(rawAngleOut->text()).arg(targetAngle).toLocal8Bit());
    QWARN("Since the rotator interface has changed, the correct rotator angle cannot be checked from the test case.");
}

void TestEkosCaptureWorkflow::testFlatManualSource()
{
    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_SET_COMBO(capture, captureTypeS, "Flat");

    // select manual flat method
    KTRY_SELECT_FLAT_METHOD(manualSourceC, false, false);
    // ensure that the filter wheel is configured
    KTRY_CAPTURE_GADGET(QComboBox, FilterPosCombo);
    // wait until filter combo box is filled
    QTRY_VERIFY_WITH_TIMEOUT(FilterPosCombo->count() > 0, 60000);
    // build a simple 1xL flat sequence
    KTRY_CAPTURE_ADD_FRAME("Flat", 1, 1, 0.0, "Luminance", imagepath);
    // build a simple 1xL light sequence
    KTRY_CAPTURE_ADD_LIGHT(1, 1, 0.0, "Red", imagepath);
    // click OK or Cancel?
    QFETCH(bool, clickModalOK);
    QFETCH(bool, clickModal2OK);
    if (clickModalOK)
    {
        // start the flat/dark/bias sequence
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
        KTRY_CLICK(capture, startB);
        // click OK in the modal dialog for covering the telescope
        CLOSE_MODAL_DIALOG(0);
        // check if one single frame is captured
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
        if (clickModal2OK)
        {
            // expect the light sequence
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
            // click OK in the modal dialog for uncovering the telescope
            CLOSE_MODAL_DIALOG(0);
            // check if one single light is captured
            KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
        }
        else
        {
            // this must not happen
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
            // click Cancel in the modal dialog for uncovering the telescope
            CLOSE_MODAL_DIALOG(1);
            // check if capturing has not been started
            KTRY_CAPTURE_GADGET(QPushButton, startB);
            // within 5 secs the job must be stopped ...
            QTRY_VERIFY_WITH_TIMEOUT(startB->icon().name() == QString("media-playback-start"), 5000);
            // ... and capturing has not been started
            QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.size() > 0, 5000);
        }
    }
    else
    {
        // this must not happen
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
        // start flats capturing
        KTRY_CLICK(capture, startB);
        // click Cancel in the modal dialog for covering the telescope
        CLOSE_MODAL_DIALOG(1);
        // check if capturing has not been started
        KTRY_CAPTURE_GADGET(QPushButton, startB);
        // within 5 secs the job mus be stopped ...
        QTRY_VERIFY_WITH_TIMEOUT(startB->icon().name() == QString("media-playback-start"), 5000);
        // ... and capturing has not been started
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.size() > 0, 5000);
    }
}


void TestEkosCaptureWorkflow::testLightPanelSource()
{
    // use the light panel simulator
    m_CaptureHelper->m_LightPanelDevice = "Light Panel Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for flats
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to the selected type so that we can set the calibration
    QFETCH(QString, frametype);
    KTRY_SET_COMBO(capture, captureTypeS, frametype);

    // select internal or external flat light
    QFETCH(bool, internalLight);
    if (internalLight == true)
        KTRY_SELECT_FLAT_METHOD(flatDeviceSourceC, false, false);
    else
        KTRY_SELECT_FLAT_METHOD(darkDeviceSourceC, false, false);
    // build a simple 1xL sequence
    KTRY_CAPTURE_ADD_FRAME(frametype, 1, 1, 0.0, "Luminance", imagepath);
    // build a simple 1xL light sequence
    KTRY_CAPTURE_ADD_LIGHT(1, 1, 0.0, "Red", imagepath);

    // start the sequence
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    KTRY_CLICK(capture, startB);
    // check if one single flat is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
}


void TestEkosCaptureWorkflow::testDustcapSource()
{
    // use the Flip Flat in simulator mode
    m_CaptureHelper->m_LightPanelDevice = "Flip Flat";

    Ekos::Manager * const ekos = Ekos::Manager::Instance();

    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(ekos->setupTab, 1000);
    QVERIFY(m_CaptureHelper->setupEkosProfile("Simulators", false));
    // start the profile
    KTRY_EKOS_CLICK(processINDIB);

    // wait until the disconnect button is enabled which shows that INDI has started
    KTRY_GADGET(ekos, QAbstractButton, disconnectB);
    QTRY_VERIFY_WITH_TIMEOUT(disconnectB->isEnabled(), 10000);

    // enable simulation
    SET_INDI_VALUE_SWITCH("Flip Flat", "SIMULATION", "ENABLE", true);
    // connect
    SET_INDI_VALUE_SWITCH("Flip Flat", "CONNECTION", "CONNECT", true);
    // park
    SET_INDI_VALUE_SWITCH("Flip Flat", "CAP_PARK", "PARK", true);
    // turn light off
    SET_INDI_VALUE_SWITCH("Flip Flat", "FLAT_LIGHT_CONTROL", "FLAT_LIGHT_OFF",
                          true);

    // Now all devices should be up and running
    QTRY_VERIFY_WITH_TIMEOUT(Ekos::Manager::Instance()->indiStatus() == Ekos::Success, 10000);

    // init the helper
    m_CaptureHelper->init();

    // receive status updates from all devices
    m_CaptureHelper->connectModules();

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for flats
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_SET_COMBO(capture, captureTypeS, "Flat");
    QTRY_VERIFY_WITH_TIMEOUT(captureTypeS->findText("Flat", Qt::MatchExactly) >= 0, 5000);

    // select frame type and internal or external flat light
    QFETCH(bool, internalLight);
    QFETCH(QString, frametype);

    if (internalLight == true)
        KTRY_SELECT_FLAT_METHOD(flatDeviceSourceC, false, false);
    else
        KTRY_SELECT_FLAT_METHOD(darkDeviceSourceC, false, false);

    // build a simple 1xL flat sequence
    KTRY_CAPTURE_ADD_FRAME(frametype, 1, 1, 0.0, "Luminance", imagepath);
    // build a simple 1xL light sequence
    KTRY_CAPTURE_ADD_LIGHT(1, 1, 0.0, "Red", imagepath);

    // start the sequence
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    KTRY_CLICK(capture, startB);
    // check if one single flat is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
}


void TestEkosCaptureWorkflow::testWallSource()
{
    // use the light panel simulator
    m_CaptureHelper->m_LightPanelDevice = "Light Panel Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for flats
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_SET_COMBO(capture, captureTypeS, "Flat");

    // select the wall as flat light source (az=90°, alt=0)
    KTRY_SELECT_FLAT_WALL(capture, "90", "0");

    // determine frame type
    QFETCH(QString, frametype);
    // build a simple 1xL sequence
    KTRY_CAPTURE_ADD_FRAME(frametype, 2, 1, 2.0, "Luminance", imagepath);
    // build a simple 1xL light sequence
    KTRY_CAPTURE_ADD_LIGHT(1, 1, 0.0, "Red", imagepath);
    // switch capture type to flat so that we can set the calibration
    captureTypeS->setCurrentText("Flat");
    // add another sequence to check if wall source may be used twice
    // select another wall position as flat light source (az=0°, alt=0)
    KTRY_SELECT_FLAT_WALL(capture, "0", "0");
    KTRY_CAPTURE_ADD_FRAME(frametype, 2, 1, 2.0, "Luminance", imagepath);

    // start the sequence
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    m_CaptureHelper->expectedMountStates.append(ISD::Mount::MOUNT_SLEWING);
    m_CaptureHelper->expectedMountStates.append(ISD::Mount::MOUNT_IDLE);
    KTRY_CLICK(capture, startB);
    // check if mount has reached the expected position
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedMountStates, 60000);
    // check if one single flat and one red frame is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);

    // reset the sequence
    KTRY_CLICK(capture, resetB);
    // restart the sequence
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    m_CaptureHelper->expectedMountStates.append(ISD::Mount::MOUNT_SLEWING);
    m_CaptureHelper->expectedMountStates.append(ISD::Mount::MOUNT_IDLE);
    KTRY_CLICK(capture, startB);
    // check if mount has reached the expected position
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedMountStates, 60000);
    // check if one single flat and one red frame is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);

}


void TestEkosCaptureWorkflow::testPreMountAndDomePark()
{
    // use the light panel simulator
    m_CaptureHelper->m_LightPanelDevice = "Light Panel Simulator";
    // use the dome simulator
    m_CaptureHelper->m_DomeDevice = "Dome Simulator";
    // default initialization
    QVERIFY(prepareTestCase());

    // QSKIP("Observatory refactoring needs to be completed until this test can be activated.");

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for flats
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_SET_COMBO(capture, captureTypeS, "Flat");

    // select internal flat light, pre-mount and but not pre-dome park
    KTRY_SELECT_FLAT_METHOD(flatDeviceSourceC, true, false);
    // determine frame type
    QFETCH(QString, frametype);
    // build a simple 1xL sequence
    KTRY_CAPTURE_ADD_FRAME(frametype, 2, 1, 2.0, "Luminance", imagepath);

    // start the sequence
    // m_CaptureHelper->expectedDomeStates.append(ISD::Dome::DOME_PARKED);
    m_CaptureHelper->expectedMountStates.append(ISD::Mount::MOUNT_PARKED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    KTRY_CLICK(capture, startB);
    // check if mount has reached the expected position
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedMountStates, 30000);
    // check if dome has reached the expected position
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedDomeStates, 30000);
    // check if one single flat is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
}

void TestEkosCaptureWorkflow::testFlatSyncFocus()
{
    // use the light panel simulator and focuser simulator
    m_CaptureHelper->m_LightPanelDevice = "Light Panel Simulator";
    m_CaptureHelper->m_FocuserDevice = "Focuser Simulator";
    // default initialization
    QVERIFY(prepareTestCase());
    // set flat sync
    Options::setFlatSyncFocus(true);
    // run autofocus
    QVERIFY(m_CaptureHelper->executeFocusing());

    Ekos::Focus *focus = Ekos::Manager::Instance()->focusModule();
    // update the initial focuser position
    KTRY_GADGET(Ekos::Manager::Instance()->focusModule(), QLineEdit, absTicksLabel);
    int focusPosition = absTicksLabel->text().toInt();
    // move the focuser 100 steps out
    KTRY_SET_SPINBOX(focus, absTicksSpin, focusPosition + 100);
    // click goto
    KTRY_CLICK(focus, startGotoB);
    // check if new position has been reached
    QTRY_VERIFY_WITH_TIMEOUT(absTicksLabel->text().toInt() == focusPosition + 100, 5000);

    // capture flats with light panel source (simplest way to test)
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for flats
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_SET_COMBO(capture, captureTypeS, "Flat");

    // select internal flat light
    KTRY_SELECT_FLAT_METHOD(flatDeviceSourceC, false, false);
    // build a simple 5xL sequence
    KTRY_CAPTURE_CONFIGURE_FLAT(2, 1, 2.0, "Luminance", imagepath);

    // start the sequence
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    KTRY_CLICK(capture, startB);
    // check if one single flat is captured
    KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
    // check if the focuser is at the determined focus position
    QTRY_VERIFY_WITH_TIMEOUT(absTicksLabel->text().toInt() == focusPosition, 5000);
}

void TestEkosCaptureWorkflow::testDarkManualCovering()
{
    // default initialization
    QVERIFY(prepareTestCase());

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // use a test directory for darks
    QString imagepath = getImageLocation()->path() + "/test";

    // switch capture type to flat so that we can set the calibration
    KTRY_CAPTURE_GADGET(QComboBox, captureTypeS);
    KTRY_CAPTURE_COMBO_SET(captureTypeS, "Dark");

    // select manual flat method
    KTRY_SELECT_FLAT_METHOD(manualSourceC, false, false);
    // build a simple 1xBlue dark sequence
    KTRY_CAPTURE_ADD_FRAME("Dark", 1, 1, 0.0, "Blue", imagepath);
    // build a simple 1xL light sequence
    KTRY_CAPTURE_ADD_LIGHT(1, 1, 0.0, "Red", imagepath);
    // shutter type
    QFETCH(int, shutter);
    // click OK or Cancel?
    QFETCH(bool, clickModalOK);
    QFETCH(bool, clickModal2OK);

    // prepare shutter settings
    QStringList shutterfulCCDs;
    QStringList shutterlessCCDs;
    if (shutter == SHUTTER_NO)
        shutterlessCCDs.append(m_CaptureHelper->m_CCDDevice);
    else if (shutter == SHUTTER_YES)
        shutterfulCCDs.append(m_CaptureHelper->m_CCDDevice);
    // set the option values
    Options::setShutterfulCCDs(shutterfulCCDs);
    Options::setShutterlessCCDs(shutterlessCCDs);

    // if the camera has a shutter, no manual cover is required
    if (clickModalOK)
    {
        // start the flat sequence
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IDLE);
        KTRY_CLICK(capture, startB);
        // if shutter type unknown, answer the the modal dialog with "No"
        if (shutter == SHUTTER_UNKNOWN)
            CLOSE_MODAL_DIALOG(1);
        // click OK in the modal dialog for covering the telescope if camera has no shutter
        if (shutter != SHUTTER_YES)
            CLOSE_MODAL_DIALOG(0);
        // check if one single flat is captured
        KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
        if (clickModal2OK)
        {
            // expect the light sequence
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
            // click OK in the modal dialog for uncovering the telescope
            CLOSE_MODAL_DIALOG(0);
            // check if one single light is captured
            KVERIFY_EMPTY_QUEUE_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates, 60000);
        }
        else
        {
            // this must not happen
            m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
            // click Cancel in the modal dialog for uncovering the telescope
            CLOSE_MODAL_DIALOG(1);
            // check if capturing has not been started
            KTRY_CAPTURE_GADGET(QPushButton, startB);
            // within 5 secs the job must be stopped ...
            QTRY_VERIFY_WITH_TIMEOUT(startB->icon().name() == QString("media-playback-start"), 5000);
            // ... and capturing has not been started
            QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.size() > 0, 5000);
        }
    }
    else
    {
        // this must not happen
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_CAPTURING);
        // start flats capturing
        KTRY_CLICK(capture, startB);
        // click Cancel in the modal dialog for covering the telescope
        CLOSE_MODAL_DIALOG(1);
        // check if capturing has not been started
        KTRY_CAPTURE_GADGET(QPushButton, startB);
        // within 5 secs the job mus be stopped ...
        QTRY_VERIFY_WITH_TIMEOUT(startB->icon().name() == QString("media-playback-start"), 5000);
        // ... and capturing has not been started
        QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.size() > 0, 5000);
    }
}


void TestEkosCaptureWorkflow::testDarksLibrary()
{
    // default initialization
    QVERIFY(prepareTestCase());

    // ensure that we know that the CCD has a shutter
    m_CaptureHelper->ensureCCDShutter(true);

    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000);

    // open the darks library dialog
    KTRY_CLICK(capture, darkLibraryB);
    QWidget *darkLibraryDialog = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(darkLibraryDialog = Ekos::Manager::Instance()->findChild<QWidget *>("DarkLibrary"), 2000);

    // select the primary train for darks capturing
    KTRY_SET_COMBO(darkLibraryDialog, opticalTrainCombo, m_CaptureHelper->m_primaryTrain);

    // set dark library values to 3x1s darks
    KTRY_SET_DOUBLESPINBOX(darkLibraryDialog, maxExposureSpin, 1);
    KTRY_SET_SPINBOX(darkLibraryDialog, countSpin, 3);
    // expect exactly 3 frames
    for (int i = 0; i < 3; i++)
        m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_IMAGE_RECEIVED);
    m_CaptureHelper->expectedCaptureStates.append(Ekos::CAPTURE_COMPLETE);
    // start
    KTRY_CLICK(darkLibraryDialog, startB);
    // wait until completion
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->expectedCaptureStates.isEmpty(), 10000);
    // check if master frame has been created
    QFileInfo destinationInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation), "darks");
    QDir destination(destinationInfo.absoluteFilePath());
    QVERIFY(m_CaptureHelper->searchFITS(destination).size() == 1);
}


/* *********************************************************************************
 *
 * Test data
 *
 * ********************************************************************************* */

void TestEkosCaptureWorkflow::testCaptureRefocusDelay_data()
{
    prepareTestData(31.0, {"Luminance:3"});
}

void TestEkosCaptureWorkflow::testCaptureRefocusHFR_data()
{
    prepareTestData(10.0, {"Luminance:6"});
}

void TestEkosCaptureWorkflow::testCaptureRefocusTemperature_data()
{
    prepareTestData(10.0, {"Luminance:6"});
}

void TestEkosCaptureWorkflow::testCaptureRefocusAbort_data()
{
    prepareTestData(31.0, {"Luminance:3"});
}

void TestEkosCaptureWorkflow::testCaptureScriptsExecution_data()
{
    QTest::addColumn<bool>("pausing");             /*!< pause between capturing */
    QTest::newRow("pausing=false") << false;
    QTest::newRow("pausing=true") << true;
}

void TestEkosCaptureWorkflow::testInitialGuidingLimitCapture_data()
{
    prepareTestData(20.0, {"Luminance:5"});
}

void TestEkosCaptureWorkflow::testFlatManualSource_data()
{
    QTest::addColumn<bool>("clickModalOK");             /*!< click "OK" on the modal dialog        */
    QTest::addColumn<bool>("clickModal2OK");            /*!< click "OK" on the second modal dialog */

    // both variants: click OK and click Cancel
    QTest::newRow("Flat, modal=true")  << true << true;
    QTest::newRow("Flat, modal=true")  << true << false;
    QTest::newRow("Flat, modal=false") << false << true;
}

void TestEkosCaptureWorkflow::testLightPanelSource_data()
{
    QTest::addColumn<QString>("frametype");              /*!< frame type (Bias, Dark, Flat)       */
    QTest::addColumn<bool>("internalLight");             /*!< use internal or external flat light */

    for (auto frametype :
            {
                "Flat", "Dark", "Bias"
            })
        for (auto internalLight : // light source integrated into the light panel?
                {
                    true, false
                })
            QTest::newRow(QString("%1, light=%2").arg(frametype).arg(internalLight ? "internal" : "external").toLatin1())
                    << frametype << internalLight;
}

void TestEkosCaptureWorkflow::testDustcapSource_data()
{
    QTest::addColumn<QString>("frametype");              /*!< frame type (Dark or Flat)           */
    QTest::addColumn<bool>("internalLight");             /*!< use internal or external flat light */

    //    QTest::newRow("Flat, light=internal") << "Flat" << true;   // flat + light source integrated into the light panel
    QTest::newRow("Flat, light=external") << "Flat" << false;  // flat + external light source used
    QTest::newRow("Dark, light=external") << "Dark" << false;  // dark + external light source used
    QTest::newRow("Bias, light=external") << "Bias" << false;  // dark + external light source used
    //    QTest::newRow("Dark") << "Dark" << false;  // dark
}

void TestEkosCaptureWorkflow::testWallSource_data()
{
    QTest::addColumn<QString>("frametype");              /*!< frame type (Dark, Flat or bias)           */

    QTest::newRow("Flat") << "Flat";
    QTest::newRow("Dark") << "Dark";
    QTest::newRow("Bias") << "Bias";
}


void TestEkosCaptureWorkflow::testPreMountAndDomePark_data()
{
    testWallSource_data();
}

void TestEkosCaptureWorkflow::testDarkManualCovering_data()
{
    QTest::addColumn<int>("shutter");                   /*!< does the CCD have a shutter?          */
    QTest::addColumn<bool>("clickModalOK");             /*!< click "OK" on the modal dialog        */
    QTest::addColumn<bool>("clickModal2OK");            /*!< click "OK" on the second modal dialog */

    // all shutter types plus both variants: click OK and click Cancel
    QTest::newRow("shutter=? modal=true") << SHUTTER_UNKNOWN << true << true;
    QTest::newRow("shutter=yes modal=true") << SHUTTER_YES << true << true;
    QTest::newRow("shutter=no modal=true") << SHUTTER_NO << true << true;
    QTest::newRow("modal=true") << SHUTTER_NO << true << false;
    QTest::newRow("modal=false") << SHUTTER_NO << false << true;
}

void TestEkosCaptureWorkflow::testCaptureWaitingForTemperature_data()
{
    QTest::addColumn<double>("initTemp");             /*!< Initial temperature value */
    QTest::addColumn<double>("targetTemp");           /*!< Target temperature value  */

    QTest::newRow("init=0 target=-5") << 0.0 << -5.0;
    QTest::newRow("init=0 target=0")  << 0.0 <<  0.0;
}


/* *********************************************************************************
 *
 * Test infrastructure
 *
 * ********************************************************************************* */

void TestEkosCaptureWorkflow::initTestCase()
{
    KVERIFY_EKOS_IS_HIDDEN();
    // limit guiding pulses to ensure that guiding deviations lead to aborted capture
    Options::setRAMaximumPulseArcSec(5.0);
    Options::setDECMaximumPulseArcSec(5.0);

    QStandardPaths::setTestModeEnabled(true);

    QFileInfo test_dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation), "test");
    destination = new QTemporaryDir(test_dir.absolutePath());
    QVERIFY(destination->isValid());
    QVERIFY(destination->autoRemove());
}

void TestEkosCaptureWorkflow::cleanupTestCase()
{
    // nothing to do since we start the INDI service for each test case
}

bool TestEkosCaptureWorkflow::prepareTestCase()
{
    // use the helper to start the profile
    KVERIFY_SUB(m_CaptureHelper->startEkosProfile());
    // prepare optical trains for testing
    m_CaptureHelper->prepareOpticalTrains();
    // prepare the mount module for testing with OAG guiding
    m_CaptureHelper->prepareMountModule(TestEkosCaptureHelper::SCOPE_FSQ85, TestEkosCaptureHelper::SCOPE_FSQ85);
    // prepare for focusing tests
    m_CaptureHelper->prepareFocusModule();
    // prepare for alignment tests
    m_CaptureHelper->prepareAlignmentModule();
    // prepare for guiding tests
    m_CaptureHelper->prepareGuidingModule();
    // prepare for capturing tests
    m_CaptureHelper->prepareCaptureModule();

    m_CaptureHelper->init();

    // clear image directory
    KVERIFY_SUB(m_CaptureHelper->getImageLocation()->removeRecursively());

    // set logging defaults for alignment
    Options::setVerboseLogging(false);
    Options::setLogToFile(false);

    // ensure that the scope is unparked
    Ekos::Mount *mount = Ekos::Manager::Instance()->mountModule();
    if (mount->parkStatus() == ISD::PARK_PARKED)
        mount->unpark();
    KTRY_VERIFY_WITH_TIMEOUT_SUB(mount->parkStatus() == ISD::PARK_UNPARKED, 30000);

    // ensure that the dome is unparked
    //    if (m_CaptureHelper->m_DomeDevice != nullptr)
    //    {
    //        Ekos::Dome *dome = Ekos::Manager::Instance()->domeModule();
    //        KVERIFY_SUB(dome != nullptr);
    //        if (dome->parkStatus() == ISD::PARK_PARKED)
    //            dome->unpark();
    //        KTRY_VERIFY_WITH_TIMEOUT_SUB(dome->parkStatus() == ISD::PARK_UNPARKED, 30000);
    //    }

    // close INDI window
    GUIManager::Instance()->close();

    // preparation successful
    return true;
}

void TestEkosCaptureWorkflow::init()
{
    // reset counters
    image_count  = 0;

    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
    // clear light panel
    m_CaptureHelper->m_LightPanelDevice = nullptr;
    // clear rotator
    m_CaptureHelper->m_RotatorDevice = nullptr;
    // disable reset jobs warning
    KMessageBox::saveDontShowAgainYesNo("reset_job_status_warning", KMessageBox::ButtonCode::No);
}

void TestEkosCaptureWorkflow::cleanup()
{
    if (Ekos::Manager::Instance()->focusModule() != nullptr)
        Ekos::Manager::Instance()->focusModule()->abort();

    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    if (capture != nullptr)
    {
        capture->abort();
        capture->clearSequenceQueue();
        KTRY_SET_CHECKBOX(capture, limitRefocusS, false);
    }

    m_CaptureHelper->cleanup();
    QVERIFY(m_CaptureHelper->shutdownEkosProfile());
    KTRY_CLOSE_EKOS();
    KVERIFY_EKOS_IS_HIDDEN();
}


bool TestEkosCaptureWorkflow::prepareCapture(int refocusLimitTime, double refocusHFR, double refocusTemp, int delay)
{
    QFETCH(double, exptime);
    QFETCH(QString, sequence);
    // test data
    // switch to capture module
    Ekos::Capture *capture = Ekos::Manager::Instance()->captureModule();
    KWRAP_SUB(KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(capture, 1000));

    // add target to path to emulate the behavior of the scheduler
    QString imagepath = getImageLocation()->path() + "/test";

    // create the destination for images
    qCInfo(KSTARS_EKOS_TEST) << "FITS path: " << imagepath;

    // set refocusing limits
    KTRY_SET_CHECKBOX_SUB(capture, limitRefocusS, (refocusLimitTime > 0));
    if (refocusLimitTime > 0)
        KTRY_SET_SPINBOX_SUB(capture, limitRefocusN, refocusLimitTime);

    KTRY_SET_CHECKBOX_SUB(capture, limitFocusHFRS, (refocusHFR > 0));
    if (refocusHFR > 0)
        KTRY_SET_DOUBLESPINBOX_SUB(capture, limitFocusHFRN, refocusHFR);

    KTRY_SET_CHECKBOX_SUB(capture, limitFocusDeltaTS, (refocusTemp > 0));
    if (refocusTemp > 0)
        KTRY_SET_DOUBLESPINBOX_SUB(capture, limitFocusDeltaTN, refocusTemp);

    // create capture sequences
    KVERIFY_SUB(m_CaptureHelper->fillCaptureSequences(target, sequence, exptime, imagepath, delay));

    // everything successfully completed
    return true;
}

void TestEkosCaptureWorkflow::prepareTestData(double exptime, QList<QString> sequenceList)
{
#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    QSKIP("Bypassing fixture test on old Qt");
    Q_UNUSED(exptime)
    Q_UNUSED(sequence)
#else
    QTest::addColumn<double>("exptime");             /*!< exposure time */
    QTest::addColumn<QString>("sequence");           /*!< list of filters */

    for (QString sequence : sequenceList)
        QTest::newRow(QString("seq=%2, exp=%1").arg(exptime).arg(sequence).toStdString().c_str()) << exptime << sequence;
#endif
}

QDir *TestEkosCaptureWorkflow::getImageLocation()
{
    if (imageLocation == nullptr || imageLocation->exists())
        imageLocation = new QDir(destination->path() + "/images");

    return imageLocation;
}

/* *********************************************************************************
 *
 * Main function
 *
 * ********************************************************************************* */

QTEST_KSTARS_WITH_GUIDER_MAIN(TestEkosCaptureWorkflow)
