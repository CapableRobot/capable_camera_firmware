#include "gnss_logger.hpp"

#include <string>
#include <cmath>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#define NUM_MODE_STRINGS    4
static const char *modeStrings[NUM_MODE_STRINGS] = {
    "n/a",
    "None",
    "2D",
    "3D"
};

GnssLogger::GnssLogger(AppOptions *opts) : Logger(opts) {};

GnssLogger::~GnssLogger() = default;

void GnssLogger::AddData(gps_data_t &data)
{
    // Organize the data and queue it for output
    json organizedData = OrganizeData(data);
    QueueData(organizedData);
}

json GnssLogger::OrganizeData(gps_data_t &data)
{
    json dataObject = json::object();

    // Add the fix string if the value is known
    if ((data.fix.mode >= 0) && (data.fix.mode < NUM_MODE_STRINGS))
    {
        dataObject["fix"] = modeStrings[data.fix.mode];
    }
    // If the value is not known, output the value directly
    else
    {
        dataObject["fix"] = data.fix.mode;
    }

    // Add the time stamp
    if ((data.set & TIME_SET) != 0)
    {
        dataObject["timestamp"] = GetDateTimeString(data.fix.time);
    }
    
    // Add lat and long data
    if (((data.set & LATLON_SET) != 0))
    {
        dataObject["latitude"] = data.fix.latitude;
        dataObject["longitude"] = data.fix.longitude;
    }

    // Add altitude
    if ((data.set & ALTITUDE_SET) != 0)
    {
        // Add altitude depending on whether we have a 2D or 3D fix
        double altitude = 
            (data.fix.mode == MODE_3D) ? 
                data.fix.altHAE : 
                data.fix.altMSL;
        dataObject["height"] = altitude;
    }

    // Add heading
    if ((data.set & TRACK_SET) != 0)
    {
        dataObject["heading"] = data.fix.track;
    }

    // Add Speed
    if ((data.set & SPEED_SET) != 0)
    {
        dataObject["speed"] = data.fix.speed;
    }

    // Add ECEF position data
    // Need to determine if data is finite, the library does not check it
    // If gpsd version 3.24 is used ECEF_SET can be used instead
    if (/*(data.set & ECEF_SET) != 0) && */
        (isfinite(data.fix.ecef.x) == true) &&
        (isfinite(data.fix.ecef.y) == true) &&
        (isfinite(data.fix.ecef.z) == true) &&
        (isfinite(data.fix.ecef.pAcc) == true))
    {
        dataObject["ecef"]["position"] = json::array({
            data.fix.ecef.x,
            data.fix.ecef.y,
            data.fix.ecef.z
        });
        dataObject["ecef"]["positionAccel"] = data.fix.ecef.pAcc;
    }
    
    // Add ECEF velocity data
    // Need to determine if data is finite, the library does not check it
    // If gpsd version 3.24 is used VECEF_SET can be used instead
    if (/*(data.set & VECEF_SET) != 0) && */
        (isfinite(data.fix.ecef.vx) == true) &&
        (isfinite(data.fix.ecef.vy) == true) &&
        (isfinite(data.fix.ecef.vz) == true) &&
        (isfinite(data.fix.ecef.vAcc) == true))
    {
        dataObject["ecef"]["velocity"] = json::array({
            data.fix.ecef.vx,
            data.fix.ecef.vy,
            data.fix.ecef.vz
        });
        dataObject["ecef"]["velocityAccel"] = data.fix.ecef.vAcc;
    }

    // Adding satellite data
    if ((data.set & SATELLITE_SET) != 0)
    {
        dataObject["satellites"]["used"] = data.satellites_used;
        dataObject["satellites"]["seen"] = data.satellites_visible;
    }

    return dataObject;
}