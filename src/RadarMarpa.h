/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *   Copyright (C) 2013-2016 by Douwe Fokkkema             df@percussion.nl*
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef _RADAR_MARPA_H_
#define _RADAR_MARPA_H_

//#include "pi_common.h"

//#include "br24radar_pi.h"
#include "Kalman.h"
#include "Matrix.h"
#include "RadarInfo.h"

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class Kalman_Filter;
class Position;
class Matrix;

#define MAX_NUMBER_OF_TARGETS (200)  // real max numer of targets is 1 less
#define TARGET_SEARCH_RADIUS1 (2)    // radius of target search area for pass 1 (on top of the size of the blob)
#define TARGET_SEARCH_RADIUS2 (15)   // radius of target search area for pass 1
#define SCAN_MARGIN (150)            // number of lines that a next scan of the target may have moved
#define SCAN_MARGIN2 (1000)          // if target is refreshed after this time you will be shure it is the next sweep
#define MAX_CONTOUR_LENGTH (601)     // defines maximal size of target contour
#define MAX_TARGET_DIAMETER (200)    // target will be set lost if diameter larger than this value
#define MIN_CONTOUR_LENGTH (8)
#define MAX_LOST_COUNT (3)  // number of sweeps that target can be missed before it is seet to lost

#define FOR_DELETION (-2)  // status of a duplicate target used to delete a target
#define LOST (-1)
#define AQUIRE0 (0)  // 0 under aquisition, first seen, no contour yet
#define AQUIRE1 (1)  // 1 under aquisition, contour found, first position FOUND
#define AQUIRE2 (2)  // 2 under aquisition, speed and course taken
#define AQUIRE3 (3)  // 3 under aquisition, speed and course verified, next time active
                     //    >=4  active

#define Q_NUM (4)  // status Q to OCPN at target status
#define T_NUM (6)  // status T to OCPN at target status
#define SPEED_HISTORY (8)
#define TARGET_SPEED_DIV_SDEV 2.
#define MAX_DUP 2                     // maximum number of sweeps a duplicate target is allowed to exist
#define STATUS_TO_OCPN (5)            // First status to be send to OCPN
#define NOISE (0.13)                  // Allowed covariance of target speed in lat and lon
                                      // critical for the performance of target tracking
                                      // lower value makes target go straight
                                      // higher values allow target to make curves
#define START_UP_SPEED (0.5)          // maximum allowed speed (m/sec) for new target, real format with .
#define DISTANCE_BETWEEN_TARGETS (4)  // minimum separation between targets

typedef int target_status;
enum OCPN_target_status {
  Q,  // aquiring
  T,  // active
  L   // lost
};

class Position {
 public:
  double lat;
  double lon;
  double dlat_dt;      // m / sec
  double dlon_dt;      // m / sec
  wxLongLong time;     // millis
  double speed_kn;
  double sd_speed_kn;  // standard deviation of the speed in knots
};

class Polar {
 public:
  int angle;
  int r;
  wxLongLong time;  // wxGetUTCTimeMillis
};

Position Polar2Pos(Polar pol, Position own_ship, double range);

class LocalPosition {
  // position in meters relative to own ship position
 public:
  double lat;
  double lon;
  double dlat_dt;  // meters per second
  double dlon_dt;
  double sd_speed_m_s;  // standard deviation of the speed m / sec
};

Polar Pos2Polar(Position p, Position own_ship, int range);

struct speed {
  double av;
  double hist[SPEED_HISTORY];
  double dif[SPEED_HISTORY];
  double sd;
  int nr;
};

enum target_process_status { UNKNOWN, NOT_FOUND_IN_PASS1 };
enum pass_n { PASS1, PASS2 };

class ArpaTarget {
 public:
  ArpaTarget(br24radar_pi* pi, RadarInfo* ri);
  ArpaTarget();
  ~ArpaTarget();
  RadarInfo* m_ri;
  br24radar_pi* m_pi;
  int target_id;
  Position X;  // holds actual position of target
  Kalman_Filter* m_kalman;
  wxLongLong t_refresh;  // time of last refresh
  target_status status;
  double speed_kn;
  double course;
  speed speeds;
  int stationary;  // number of sweeps target was stationary
  bool arpa;
  int lost_count;
  int duplicate_count;
  bool check_for_duplicate;
  target_process_status pass1_result;
  pass_n pass_nr;
  Polar contour[MAX_CONTOUR_LENGTH + 1];  // contour of target, only valid immediately after finding it
  Polar expected;
  int contour_length;
  Polar max_angle, min_angle, max_r, min_r;  // charasterictics of contour
  int GetContour(Polar* p);
  void set(br24radar_pi* pi, RadarInfo* ri);
  bool FindNearestContour(Polar* pol, int dist);
  bool FindContourFromInside(Polar* p);
  bool GetTarget(Polar* pol, int dist);
  void RefreshTarget(int dist);
  void PassARPAtoOCPN(Polar* p, OCPN_target_status s);
  void SetStatusLost();
  void ResetPixels();
  void GetSpeed();
  bool Pix(int ang, int rad);
  bool MultiPix(int ang, int rad);
};

class RadarArpa {
 public:
  RadarArpa(br24radar_pi* pi, RadarInfo* ri);
  ~RadarArpa();

  ArpaTarget* m_targets[MAX_NUMBER_OF_TARGETS];
  br24radar_pi* m_pi;
  RadarInfo* m_ri;
  int number_of_targets;
  int radar_lost_count;  // all targets will be deleted when radar not seen
  void CalculateCentroid(ArpaTarget* t);
  void DrawContour(ArpaTarget* t);
  void DrawArpaTargets();
  void RefreshArpaTargets();
  void AquireNewTarget(Position p, int status);
  void AquireNewTarget(Polar pol, int status, int* target_i);
  void DeleteAllTargets();
  bool Pix(int ang, int rad);
  bool MultiPix(int ang, int rad);
};

PLUGIN_END_NAMESPACE

#endif
