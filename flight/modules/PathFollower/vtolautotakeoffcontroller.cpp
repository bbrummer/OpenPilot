/*
 ******************************************************************************
 *
 * @file       vtollandcontroller.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Vtol landing controller loop
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

extern "C" {
#include <openpilot.h>

#include <callbackinfo.h>

#include <math.h>
#include <pid.h>
#include <CoordinateConversions.h>
#include <sin_lookup.h>
#include <pathdesired.h>
#include <paths.h>
#include "plans.h"
#include <sanitycheck.h>

#include <homelocation.h>
#include <accelstate.h>
#include <vtolpathfollowersettings.h>
#include <flightstatus.h>
#include <flightmodesettings.h>
#include <pathstatus.h>
#include <positionstate.h>
#include <velocitystate.h>
#include <velocitydesired.h>
#include <stabilizationdesired.h>
#include <airspeedstate.h>
#include <attitudestate.h>
#include <takeofflocation.h>
#include <poilocation.h>
#include <manualcontrolcommand.h>
#include <systemsettings.h>
#include <stabilizationbank.h>
#include <stabilizationdesired.h>
#include <vtolselftuningstats.h>
#include <pathsummary.h>
}

// C++ includes
#include "vtolautotakeoffcontroller.h"
#include "pathfollowerfsm.h"
#include "vtolautotakeofffsm.h"
#include "pidcontroldown.h"

// Private constants

// pointer to a singleton instance
VtolAutoTakeoffController *VtolAutoTakeoffController::p_inst = 0;

VtolAutoTakeoffController::VtolAutoTakeoffController()
    : fsm(0), vtolPathFollowerSettings(0), mActive(false)
{}

// Called when mode first engaged
void VtolAutoTakeoffController::Activate(void)
{
    if (!mActive) {
        mActive = true;
        SettingsUpdated();
        fsm->Activate();
        controlDown.Activate();
        controlNE.Activate();
    }
}

uint8_t VtolAutoTakeoffController::IsActive(void)
{
    return mActive;
}

uint8_t VtolAutoTakeoffController::Mode(void)
{
    return PATHDESIRED_MODE_AUTOTAKEOFF;
}

// Objective updated in pathdesired, e.g. same flight mode but new target velocity
void VtolAutoTakeoffController::ObjectiveUpdated(void)
{
    // Set the objective's target velocity

    if (flightStatus->ControlChain.PathPlanner != FLIGHTSTATUS_CONTROLCHAIN_TRUE) {
        controlDown.UpdateVelocitySetpoint(pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_DOWN]);
        controlNE.UpdateVelocitySetpoint(pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_NORTH],
                                         pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_EAST]);
        controlNE.UpdatePositionSetpoint(pathDesired->End.North, pathDesired->End.East);
        controlDown.UpdatePositionSetpoint(pathDesired->End.Down);
        fsm->setControlState((StatusVtolAutoTakeoffControlStateOptions)pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_CONTROLSTATE]);
    } else {
        float velocity_down;
        FlightModeSettingsAutoTakeOffVelocityGet(&velocity_down);
        controlDown.UpdateVelocitySetpoint(-velocity_down);
        controlNE.UpdateVelocitySetpoint(0.0f, 0.0f);
        // pathplanner mode the waypoint provides the final destination including altitude. Takeoff would
        // often be waypoint 0, in which case the starting location is the current location, and the end location
        // is the waypoint location which really needs to be the same as the end in north and east. If it is not,
        // it will fly to that location the during ascent.
        // Note in pathplanner mode we use the start location which is the current initial location as the
        // target NE to control.  Takeoff only ascends vertically.
        controlNE.UpdatePositionSetpoint(pathDesired->Start.North, pathDesired->Start.East);
        // Sanity check that the end location is at least a reasonable height above the start location in the down direction
        float autotakeoff_height = pathDesired->Start.Down - pathDesired->End.Down;
        if (autotakeoff_height < 2.0f) {
            pathDesired->End.Down = pathDesired->Start.Down - 2.0f;
        }
        controlDown.UpdatePositionSetpoint(pathDesired->End.Down); // the altitude is set by the end location.
        fsm->setControlState(STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_INITIATE);
    }
}
void VtolAutoTakeoffController::Deactivate(void)
{
    if (mActive) {
        mActive = false;
        fsm->Inactive();
        controlDown.Deactivate();
        controlNE.Deactivate();
    }
}

// AutoTakeoff Uses different vertical velocity PID.
void VtolAutoTakeoffController::SettingsUpdated(void)
{
    const float dT = vtolPathFollowerSettings->UpdatePeriod / 1000.0f;

    controlNE.UpdateParameters(vtolPathFollowerSettings->HorizontalVelPID.Kp,
                               vtolPathFollowerSettings->HorizontalVelPID.Ki,
                               vtolPathFollowerSettings->HorizontalVelPID.Kd,
                               vtolPathFollowerSettings->HorizontalVelPID.ILimit,
                               dT,
                               vtolPathFollowerSettings->HorizontalVelMax);


    controlNE.UpdatePositionalParameters(vtolPathFollowerSettings->HorizontalPosP);
    controlNE.UpdateCommandParameters(-vtolPathFollowerSettings->MaxRollPitch, vtolPathFollowerSettings->MaxRollPitch, vtolPathFollowerSettings->VelocityFeedforward);

    controlDown.UpdateParameters(vtolPathFollowerSettings->AutoTakeoffVerticalVelPID.Kp,
                                 vtolPathFollowerSettings->AutoTakeoffVerticalVelPID.Ki,
                                 vtolPathFollowerSettings->AutoTakeoffVerticalVelPID.Kd,
                                 vtolPathFollowerSettings->AutoTakeoffVerticalVelPID.Beta,
                                 dT,
                                 vtolPathFollowerSettings->VerticalVelMax);
    controlDown.UpdatePositionalParameters(vtolPathFollowerSettings->VerticalPosP);
    VtolSelfTuningStatsData vtolSelfTuningStats;
    VtolSelfTuningStatsGet(&vtolSelfTuningStats);
    controlDown.UpdateNeutralThrust(vtolSelfTuningStats.NeutralThrustOffset + vtolPathFollowerSettings->ThrustLimits.Neutral);
    controlDown.SetThrustLimits(vtolPathFollowerSettings->ThrustLimits.Min, vtolPathFollowerSettings->ThrustLimits.Max);
    fsm->SettingsUpdated();
}

// AutoTakeoff Uses a different FSM to its parent
int32_t VtolAutoTakeoffController::Initialize(VtolPathFollowerSettingsData *ptr_vtolPathFollowerSettings)
{
    PIOS_Assert(ptr_vtolPathFollowerSettings);
    vtolPathFollowerSettings = ptr_vtolPathFollowerSettings;

    if (fsm == 0) {
        fsm = VtolAutoTakeoffFSM::instance();
        VtolAutoTakeoffFSM::instance()->Initialize(vtolPathFollowerSettings, pathDesired, flightStatus);
        controlDown.Initialize(fsm);
    }
    return 0;
}


void VtolAutoTakeoffController::UpdateVelocityDesired()
{
    VelocityStateData velocityState;

    VelocityStateGet(&velocityState);
    VelocityDesiredData velocityDesired;
    PositionStateData positionState;
    PositionStateGet(&positionState);

    if (fsm->PositionHoldState()) {
        controlDown.UpdatePositionState(positionState.Down);
        controlDown.ControlPosition();
    }

    controlDown.UpdateVelocityState(velocityState.Down);
    controlNE.UpdateVelocityState(velocityState.North, velocityState.East);

    controlNE.UpdatePositionState(positionState.North, positionState.East);
    controlNE.ControlPosition();

    velocityDesired.Down  = controlDown.GetVelocityDesired();
    float north, east;
    controlNE.GetVelocityDesired(&north, &east);
    velocityDesired.North = north;
    velocityDesired.East  = east;

    // update pathstatus
    pathStatus->error     = 0.0f;
    pathStatus->fractional_progress = 0.0f;
    if (fsm->PositionHoldState()) {
        pathStatus->fractional_progress = 1.0f;
        // note if the takeoff waypoint and the launch position are significantly different
        // the above check might need to expand to assessment of north and east.
    }
    pathStatus->path_direction_north = velocityDesired.North;
    pathStatus->path_direction_east  = velocityDesired.East;
    pathStatus->path_direction_down  = velocityDesired.Down;

    pathStatus->correction_direction_north = velocityDesired.North - velocityState.North;
    pathStatus->correction_direction_east  = velocityDesired.East - velocityState.East;
    pathStatus->correction_direction_down  = velocityDesired.Down - velocityState.Down;

    VelocityDesiredSet(&velocityDesired);
}

int8_t VtolAutoTakeoffController::UpdateStabilizationDesired(bool yaw_attitude, float yaw_direction)
{
    uint8_t result = 1;
    StabilizationDesiredData stabDesired;
    AttitudeStateData attitudeState;
    StabilizationBankData stabSettings;
    float northCommand;
    float eastCommand;

    StabilizationDesiredGet(&stabDesired);
    AttitudeStateGet(&attitudeState);
    StabilizationBankGet(&stabSettings);

    controlNE.GetNECommand(&northCommand, &eastCommand);
    stabDesired.Thrust = controlDown.GetDownCommand();

    float angle_radians = DEG2RAD(attitudeState.Yaw);
    float cos_angle     = cosf(angle_radians);
    float sine_angle    = sinf(angle_radians);
    float maxPitch = vtolPathFollowerSettings->MaxRollPitch;
    stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Pitch = boundf(-northCommand * cos_angle - eastCommand * sine_angle, -maxPitch, maxPitch);
    stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Roll = boundf(-northCommand * sine_angle + eastCommand * cos_angle, -maxPitch, maxPitch);

    ManualControlCommandData manualControl;
    ManualControlCommandGet(&manualControl);

    if (yaw_attitude) {
        stabDesired.StabilizationMode.Yaw = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.Yaw = yaw_direction;
    } else {
        stabDesired.StabilizationMode.Yaw = STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK;
        stabDesired.Yaw = stabSettings.MaximumRate.Yaw * manualControl.Yaw;
    }

    // default thrust mode to cruise control
    stabDesired.StabilizationMode.Thrust = STABILIZATIONDESIRED_STABILIZATIONMODE_CRUISECONTROL;

    fsm->ConstrainStabiDesired(&stabDesired); // excludes thrust
    StabilizationDesiredSet(&stabDesired);

    return result;
}

void VtolAutoTakeoffController::UpdateAutoPilot()
{
    fsm->Update();

    UpdateVelocityDesired();

    // yaw behaviour is configurable in vtolpathfollower, select yaw control algorithm
    bool yaw_attitude = true;
    float yaw = 0.0f;

    fsm->GetYaw(yaw_attitude, yaw);

    int8_t result = UpdateStabilizationDesired(yaw_attitude, yaw);

    if (!result) {
        fsm->Abort();
    }

    if (fsm->GetCurrentState() == PFFSM_STATE_DISARMED) {
        setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);
    }

    PathStatusSet(pathStatus);
}

void VtolAutoTakeoffController::setArmedIfChanged(uint8_t val)
{
    if (flightStatus->Armed != val) {
        flightStatus->Armed = val;
        FlightStatusSet(flightStatus);
    }
}
