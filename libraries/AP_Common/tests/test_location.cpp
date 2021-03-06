#include <AP_gtest.h>
#include <AP_Common/Location.h>
#include <AP_Math/AP_Math.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_Terrain/AP_Terrain.h>

const AP_HAL::HAL& hal = AP_HAL::get_HAL();


class DummyAHRS: AP_AHRS_NavEKF {
public:
    DummyAHRS(uint8_t flags = 0) :
    AP_AHRS_NavEKF(flags) {};
    void unset_home() { _home_is_set = false; };
    bool set_home(const Location &loc) override WARN_IF_UNUSED {
        // check location is valid
        if (loc.lat == 0 && loc.lng == 0 && loc.alt == 0) {
            return false;
        }
        if (!loc.check_latlng()) {
            return false;
        }
        // home must always be global frame at the moment as .alt is
        // accessed directly by the vehicles and they may not be rigorous
        // in checking the frame type.
        Location tmp = loc;
        if (!tmp.change_alt_frame(Location::AltFrame::ABSOLUTE)) {
            return false;
        }

        _home = tmp;
        _home_is_set = true;
        return true;
    };

    bool set_origin(const Location &loc) override WARN_IF_UNUSED {
        _origin = loc;
        _origin_is_set = true;
        return true;
    };

    // returns the inertial navigation origin in lat/lon/alt
    bool get_origin(Location &ret) const override WARN_IF_UNUSED {
        if (_origin_is_set) {
            ret = _origin;
            return true;
        } else {
            return false;
        }

    };
    Location _origin;
    bool _origin_is_set;
};

class DummyVehicle {
public:
    bool start_cmd(const AP_Mission::Mission_Command& cmd) { return true; };
    bool verify_cmd(const AP_Mission::Mission_Command& cmd) { return true; };
    void mission_complete() { };
    DummyAHRS ahrs{AP_AHRS_NavEKF::FLAG_ALWAYS_USE_EKF};

    AP_Mission mission{
        FUNCTOR_BIND_MEMBER(&DummyVehicle::start_cmd, bool, const AP_Mission::Mission_Command &),
        FUNCTOR_BIND_MEMBER(&DummyVehicle::verify_cmd, bool, const AP_Mission::Mission_Command &),
        FUNCTOR_BIND_MEMBER(&DummyVehicle::mission_complete, void)};
    AP_Terrain terrain{mission};
};

static DummyVehicle vehicle;

#define EXPECT_VECTOR2F_EQ(v1, v2)              \
do {                                        \
EXPECT_FLOAT_EQ(v1[0], v2[0]);          \
EXPECT_FLOAT_EQ(v1[1], v2[1]);          \
} while (false);

#define EXPECT_VECTOR3F_EQ(v1, v2)              \
do {                                        \
EXPECT_FLOAT_EQ(v1[0], v2[0]);          \
EXPECT_FLOAT_EQ(v1[1], v2[1]);          \
EXPECT_FLOAT_EQ(v1[2], v2[2]);          \
} while (false);

#define EXPECT_VECTOR2F_NEAR(v1, v2, acc)              \
do {                                        \
EXPECT_NEAR(v1[0], v2[0], acc);          \
EXPECT_NEAR(v1[1], v2[1], acc);          \
} while (false);

#define EXPECT_VECTOR3F_NEAR(v1, v2, acc)              \
do {                                        \
EXPECT_NEAR(v1[0], v2[0], acc);          \
EXPECT_NEAR(v1[1], v2[1], acc);          \
EXPECT_NEAR(v1[2], v2[2], acc);          \
} while (false);

TEST(Location, Tests)
{
    Location test_location;
    EXPECT_TRUE(test_location.is_zero());
    EXPECT_FALSE(test_location.initialised());
    const Location test_home{-35362938, 149165085, 100, Location::AltFrame::ABSOLUTE};
    EXPECT_EQ(-35362938, test_home.lat);
    EXPECT_EQ(149165085, test_home.lng);
    EXPECT_EQ(100, test_home.alt);
    EXPECT_EQ(0, test_home.relative_alt);
    EXPECT_EQ(0, test_home.terrain_alt);
    EXPECT_EQ(0, test_home.origin_alt);
    EXPECT_EQ(0, test_home.loiter_ccw);
    EXPECT_EQ(0, test_home.loiter_xtrack);
    EXPECT_TRUE(test_home.initialised());

    const Vector3f test_vect{-42, 42, 0};
    Location test_location3{test_vect, Location::AltFrame::ABOVE_HOME};
    EXPECT_EQ(0, test_location3.lat);
    EXPECT_EQ(0, test_location3.lng);
    EXPECT_EQ(0, test_location3.alt);
    EXPECT_EQ(1, test_location3.relative_alt);
    EXPECT_EQ(0, test_location3.terrain_alt);
    EXPECT_EQ(0, test_location3.origin_alt);
    EXPECT_EQ(0, test_location3.loiter_ccw);
    EXPECT_EQ(0, test_location3.loiter_xtrack);
    EXPECT_FALSE(test_location3.initialised());
    // EXPECT_EXIT(test_location3.change_alt_frame(Location::AltFrame::ABSOLUTE), PANIC something); // TODO check PANIC

    test_location3.set_alt_cm(-420, Location::AltFrame::ABSOLUTE);
    EXPECT_EQ(-420, test_location3.alt);
    EXPECT_EQ(0, test_location3.relative_alt);
    EXPECT_EQ(0, test_location3.terrain_alt);
    EXPECT_EQ(0, test_location3.origin_alt);
    EXPECT_EQ(Location::AltFrame::ABSOLUTE, test_location3.get_alt_frame());

    test_location3.set_alt_cm(420, Location::AltFrame::ABOVE_HOME);
    EXPECT_EQ(420, test_location3.alt);
    EXPECT_EQ(1, test_location3.relative_alt);
    EXPECT_EQ(0, test_location3.terrain_alt);
    EXPECT_EQ(0, test_location3.origin_alt);
    EXPECT_EQ(Location::AltFrame::ABOVE_HOME, test_location3.get_alt_frame());

    test_location3.set_alt_cm(-420, Location::AltFrame::ABOVE_ORIGIN);
    EXPECT_EQ(-420, test_location3.alt);
    EXPECT_EQ(0, test_location3.relative_alt);
    EXPECT_EQ(0, test_location3.terrain_alt);
    EXPECT_EQ(1, test_location3.origin_alt);
    EXPECT_EQ(Location::AltFrame::ABOVE_ORIGIN, test_location3.get_alt_frame());

    test_location3.set_alt_cm(420, Location::AltFrame::ABOVE_TERRAIN);
    EXPECT_EQ(420, test_location3.alt);
    EXPECT_EQ(1, test_location3.relative_alt);
    EXPECT_EQ(1, test_location3.terrain_alt);
    EXPECT_EQ(0, test_location3.origin_alt);
    EXPECT_EQ(Location::AltFrame::ABOVE_TERRAIN, test_location3.get_alt_frame());

    // No TERRAIN, NO HOME, NO ORIGIN
    for (auto current_frame = Location::AltFrame::ABSOLUTE;
         current_frame <= Location::AltFrame::ABOVE_TERRAIN;
         current_frame = static_cast<Location::AltFrame>(
                 (uint8_t) current_frame + 1)) {
        for (auto desired_frame = Location::AltFrame::ABSOLUTE;
             desired_frame <= Location::AltFrame::ABOVE_TERRAIN;
             desired_frame = static_cast<Location::AltFrame>(
                     (uint8_t) desired_frame + 1)) {
            test_location3.set_alt_cm(420, current_frame);
            if (current_frame == desired_frame) {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
                continue;
            }
            if (current_frame == Location::AltFrame::ABOVE_TERRAIN
                    || desired_frame == Location::AltFrame::ABOVE_TERRAIN) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else if (current_frame == Location::AltFrame::ABOVE_ORIGIN
                    || desired_frame == Location::AltFrame::ABOVE_ORIGIN) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else if (current_frame == Location::AltFrame::ABOVE_HOME
                    || desired_frame == Location::AltFrame::ABOVE_HOME) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
            }
        }
    }
    // NO TERRAIN, NO ORIGIN
    EXPECT_TRUE(vehicle.ahrs.set_home(test_home));
    for (auto current_frame = Location::AltFrame::ABSOLUTE;
         current_frame <= Location::AltFrame::ABOVE_TERRAIN;
         current_frame = static_cast<Location::AltFrame>(
                 (uint8_t) current_frame + 1)) {
        for (auto desired_frame = Location::AltFrame::ABSOLUTE;
             desired_frame <= Location::AltFrame::ABOVE_TERRAIN;
             desired_frame = static_cast<Location::AltFrame>(
                     (uint8_t) desired_frame + 1)) {
            test_location3.set_alt_cm(420, current_frame);
            if (current_frame == desired_frame) {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
                continue;
            }
            if (current_frame == Location::AltFrame::ABOVE_TERRAIN
                    || desired_frame == Location::AltFrame::ABOVE_TERRAIN) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else if (current_frame == Location::AltFrame::ABOVE_ORIGIN
                    || desired_frame == Location::AltFrame::ABOVE_ORIGIN) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
            }

        }
    }
    // NO Origin
    Location::set_terrain(&vehicle.terrain);
    for (auto current_frame = Location::AltFrame::ABSOLUTE;
         current_frame <= Location::AltFrame::ABOVE_TERRAIN;
         current_frame = static_cast<Location::AltFrame>(
                 (uint8_t) current_frame + 1)) {
        for (auto desired_frame = Location::AltFrame::ABSOLUTE;
             desired_frame <= Location::AltFrame::ABOVE_TERRAIN;
             desired_frame = static_cast<Location::AltFrame>(
                     (uint8_t) desired_frame + 1)) {
            test_location3.set_alt_cm(420, current_frame);
            if (current_frame == desired_frame) {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
                continue;
            }
            if (current_frame == Location::AltFrame::ABOVE_ORIGIN
                    || desired_frame == Location::AltFrame::ABOVE_ORIGIN) {
                EXPECT_FALSE(test_location3.change_alt_frame(desired_frame));
            } else {
                EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
            }
        }
    }

    Vector2f test_vec2;
    EXPECT_FALSE(test_home.get_vector_xy_from_origin_NE(test_vec2));
    Vector3f test_vec3;
    EXPECT_FALSE(test_home.get_vector_from_origin_NEU(test_vec3));

    Location test_origin = test_home;
    test_origin.offset(2, 2);
    EXPECT_TRUE(vehicle.ahrs.set_origin(test_origin));
    const Vector3f test_vecto{200, 200, 10};
    const Location test_location4{test_vecto, Location::AltFrame::ABOVE_ORIGIN};
    EXPECT_EQ(-35362580, test_location4.lat);
    EXPECT_EQ(149165445, test_location4.lng);
    EXPECT_EQ(10, test_location4.alt);
    EXPECT_EQ(0, test_location4.relative_alt);
    EXPECT_EQ(0, test_location4.terrain_alt);
    EXPECT_EQ(1, test_location4.origin_alt);
    EXPECT_EQ(0, test_location4.loiter_ccw);
    EXPECT_EQ(0, test_location4.loiter_xtrack);
    EXPECT_TRUE(test_location4.initialised());

    for (auto current_frame = Location::AltFrame::ABSOLUTE;
         current_frame <= Location::AltFrame::ABOVE_TERRAIN;
         current_frame = static_cast<Location::AltFrame>(
                 (uint8_t) current_frame + 1)) {
        for (auto desired_frame = Location::AltFrame::ABSOLUTE;
             desired_frame <= Location::AltFrame::ABOVE_TERRAIN;
             desired_frame = static_cast<Location::AltFrame>(
                     (uint8_t) desired_frame + 1)) {
            test_location3.set_alt_cm(420, current_frame);
            EXPECT_TRUE(test_location3.change_alt_frame(desired_frame));
        }
    }
    EXPECT_TRUE(test_home.get_vector_xy_from_origin_NE(test_vec2));
    const float ACCURACY = 1; // TODO: WTF : 1m accuracy ?
    EXPECT_VECTOR2F_NEAR(Vector2f(-200, -200), test_vec2, ACCURACY);
    EXPECT_TRUE(test_home.get_vector_from_origin_NEU(test_vec3));
    EXPECT_VECTOR2F_NEAR(Vector3f(-200, -200, 0), test_vec3, ACCURACY);
    vehicle.ahrs.unset_home();
    const Location test_location_empty{test_vect, Location::AltFrame::ABOVE_HOME};
    EXPECT_FALSE(test_location_empty.get_vector_from_origin_NEU(test_vec3));
}

TEST(Location, Distance)
{
    const Location test_home{-35362938, 149165085, 100, Location::AltFrame::ABSOLUTE};
    const Location test_home2{-35363938, 149165085, 100, Location::AltFrame::ABSOLUTE};
    EXPECT_FLOAT_EQ(11.131885, test_home.get_distance(test_home2));
    EXPECT_FLOAT_EQ(0, test_home.get_distance(test_home));
    EXPECT_VECTOR2F_EQ(Vector2f(0, 0), test_home.get_distance_NE(test_home));
    EXPECT_VECTOR2F_EQ(Vector2f(-11.131885, 0), test_home.get_distance_NE(test_home2));
    EXPECT_VECTOR2F_EQ(Vector3f(0, 0, 0), test_home.get_distance_NED(test_home));
    EXPECT_VECTOR2F_EQ(Vector3f(-11.131885, 0, 0), test_home.get_distance_NED(test_home2));
    Location test_loc = test_home;
    test_loc.offset(-11.131885, 0);
    EXPECT_TRUE(test_loc.same_latlon_as(test_home2));
    test_loc.offset_bearing(0, 11.131885);
    EXPECT_TRUE(test_loc.same_latlon_as(test_home));

    test_loc.offset_bearing_and_pitch(0, 2, -11.14);
    EXPECT_TRUE(test_loc.same_latlon_as(test_home2));
    EXPECT_EQ(62, test_loc.alt);

    test_loc = Location(-35362633, 149165085, 0, Location::AltFrame::ABOVE_HOME);
    int32_t bearing = test_home.get_bearing_to(test_loc);
    EXPECT_EQ(0, bearing);

    test_loc = Location(-35363711, 149165085, 0, Location::AltFrame::ABOVE_HOME);
    bearing = test_home.get_bearing_to(test_loc);
    EXPECT_EQ(18000, bearing);

    test_loc = Location(-35362938, 149166085, 0, Location::AltFrame::ABOVE_HOME);
    bearing = test_home.get_bearing_to(test_loc);
    EXPECT_EQ(9000, bearing);

    test_loc = Location(-35362938, 149164085, 0, Location::AltFrame::ABOVE_HOME);
    bearing = test_home.get_bearing_to(test_loc);
    EXPECT_EQ(27000, bearing);

    test_loc = Location(-35361938, 149164085, 0, Location::AltFrame::ABOVE_HOME);
    bearing = test_home.get_bearing_to(test_loc);
    EXPECT_EQ(31503, bearing);
    const float bearing_rad = test_home.get_bearing(test_loc);
    EXPECT_FLOAT_EQ(radians(315.03), bearing_rad);

}

TEST(Location, Sanitize)
{
    const Location test_home{-35362938, 149165085, 100, Location::AltFrame::ABSOLUTE};
    Location test_loc;
    test_loc.set_alt_cm(0, Location::AltFrame::ABOVE_HOME);
    EXPECT_TRUE(test_loc.sanitize(test_home));
    EXPECT_TRUE(test_loc.same_latlon_as(test_home));
    EXPECT_EQ(test_home.alt, test_loc.alt);
    test_loc = Location(91*1e7, 0, 0, Location::AltFrame::ABSOLUTE);
    EXPECT_TRUE(test_loc.sanitize(test_home));
    EXPECT_TRUE(test_loc.same_latlon_as(test_home));
    EXPECT_NE(test_home.alt, test_loc.alt);
    test_loc = Location(0, 181*1e7, 0, Location::AltFrame::ABSOLUTE);
    EXPECT_TRUE(test_loc.sanitize(test_home));
    EXPECT_TRUE(test_loc.same_latlon_as(test_home));
    EXPECT_NE(test_home.alt, test_loc.alt);
    test_loc = Location(42*1e7, 42*1e7, 420, Location::AltFrame::ABSOLUTE);
    EXPECT_FALSE(test_loc.sanitize(test_home));
    EXPECT_FALSE(test_loc.same_latlon_as(test_home));
    EXPECT_NE(test_home.alt, test_loc.alt);
}

TEST(Location, Line)
{
    const Location test_home{35362938, 149165085, 100, Location::AltFrame::ABSOLUTE};
    const Location test_wp_last{35362960, 149165085, 100, Location::AltFrame::ABSOLUTE};
    Location test_wp{35362940, 149165085, 100, Location::AltFrame::ABSOLUTE};
    EXPECT_FALSE(test_wp.past_interval_finish_line(test_home, test_wp_last));
    EXPECT_TRUE(test_wp.past_interval_finish_line(test_home, test_home));
    test_wp.lat = 35362970;
    EXPECT_TRUE(test_wp.past_interval_finish_line(test_home, test_wp_last));
}

AP_GTEST_MAIN()
