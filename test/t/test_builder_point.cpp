
#include <test.hpp>
#include <test_point.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

struct point_handler {

    constexpr static const unsigned int max_geometric_attributes = 0;

    std::vector<vtzero::point> data;

    static vtzero::point convert(const vtzero::unscaled_point& p) noexcept {
        return {p.x, p.y};
    }

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

struct point_handler_3d {

    constexpr static const unsigned int max_geometric_attributes = 0;

    std::vector<test_point_3d> data;

    static test_point_3d convert(const vtzero::unscaled_point& p) noexcept {
        return {p.x, p.y, p.z};
    }

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const test_point_3d& point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

static void test_point_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        SECTION("add point using coordinates / property using key/value") {
            fbuilder.add_point(10, 20);
            if (with_prop) {
                fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
            }
        }

        SECTION("add point using vtzero::point / property using key/value") {
            fbuilder.add_point(vtzero::point{10, 20});
            if (with_prop) {
                fbuilder.add_property("foo", vtzero::encoded_property_value{22});
            }
        }

        SECTION("add point using test_point_2d / property using property") {
            vtzero::encoded_property_value pv{3.5};
            vtzero::property p{"foo", vtzero::property_value{pv.data()}};
            fbuilder.add_point(test_point_2d{10, 20});
            if (with_prop) {
                fbuilder.add_property(p);
            }
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    point_handler handler;
    feature.decode_point_geometry(handler);

    const std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Point builder without id/without properties") {
    test_point_builder(false, false);
}

TEST_CASE("Point builder without id/with properties") {
    test_point_builder(false, true);
}

TEST_CASE("Point builder with id/without properties") {
    test_point_builder(true, false);
}

TEST_CASE("Point builder with id/with properties") {
    test_point_builder(true, true);
}

static void test_point_builder_vt3(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        SECTION("add point using coordinates / property using key/string value") {
            fbuilder.add_point(10, 20);
            if (with_prop) {
                fbuilder.add_scalar_attribute("foo", vtzero::data_view{"bar"});
            }
        }

        SECTION("add point using vtzero::point / property using key/int value") {
            fbuilder.add_point(vtzero::point{10, 20});
            if (with_prop) {
                fbuilder.add_scalar_attribute("foo", 22);
            }
        }

        SECTION("add point using test_point_2d / property using key/double value") {
            fbuilder.add_point(test_point_2d{10, 20});
            if (with_prop) {
                fbuilder.add_scalar_attribute("foo", 3.5);
            }
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == (with_id ? 17 : 0));
    REQUIRE_FALSE(feature.has_3d_geometry());

    point_handler handler;
    feature.decode_point_geometry(handler);

    const std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Point builder without id/without properties (vt3)") {
    test_point_builder_vt3(false, false);
}

TEST_CASE("Point builder without id/with properties (vt3)") {
    test_point_builder_vt3(false, true);
}

TEST_CASE("Point builder with id/without properties (vt3)") {
    test_point_builder_vt3(true, false);
}

TEST_CASE("Point builder with id/with properties (vt3)") {
    test_point_builder_vt3(true, true);
}

TEST_CASE("Point builder with 3d point") {
    vtzero::scaling scaling{10, 1.0, 3.0};
    const auto elev = scaling.encode(30.0);

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    lbuilder.set_elevation_scaling(scaling);
    REQUIRE(lbuilder.elevation_scaling() == scaling);
    {
        vtzero::point_feature_builder<3, true> fbuilder{lbuilder};
        fbuilder.set_integer_id(17);
        fbuilder.add_point(vtzero::unscaled_point{10, 20, elev});
        fbuilder.commit();
    }
    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 17);
    REQUIRE(feature.has_3d_geometry());

    point_handler_3d handler;
    feature.decode_point_geometry(handler);
    REQUIRE(handler.data.size() == 1);

    const auto p = handler.data[0];
    REQUIRE(p.x == 10);
    REQUIRE(p.y == 20);
    REQUIRE(layer.elevation_scaling() == scaling);
    REQUIRE(layer.elevation_scaling().decode(p.elev) == Approx(30.0));
}

TEST_CASE("Calling add_points() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_points(0), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_points(1ul << 29u), const assert_error&);
    }
}

static void test_multipoint_builder(bool with_id, bool with_prop) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_integer_id(17);
    }

    fbuilder.add_points(3);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(vtzero::point{20, 30});
    fbuilder.set_point(test_point_2d{30, 40});

    if (with_prop) {
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    point_handler handler;
    feature.decode_point_geometry(handler);

    const std::vector<vtzero::point> result = {{10, 20}, {20, 30}, {30, 40}};
    REQUIRE(handler.data == result);
}


TEST_CASE("Multipoint builder without id/without properties") {
    test_multipoint_builder(false, false);
}

TEST_CASE("Multipoint builder without id/with properties") {
    test_multipoint_builder(false, true);
}

TEST_CASE("Multipoint builder with id/without properties") {
    test_multipoint_builder(true, false);
}

TEST_CASE("Multipoint builder with id/with properties") {
    test_multipoint_builder(true, true);
}

TEST_CASE("Calling add_point() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    fbuilder.add_point(10, 20);

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(10, 20), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
    SECTION("set_point()") {
        REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const assert_error&);
    }
}

TEST_CASE("Calling point_2d_feature_builder::set_point() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.set_point(10, 10), const assert_error&);
}

TEST_CASE("Calling add_points() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    fbuilder.add_points(2);

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(10, 20), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
}

TEST_CASE("Calling point_2d_feature_builder::set_point() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    fbuilder.add_points(2);
    fbuilder.set_point(10, 20);
    fbuilder.set_point(20, 20);
    REQUIRE_THROWS_AS(fbuilder.set_point(30, 20), const assert_error&);
}

TEST_CASE("Add points from container") {
    const std::vector<vtzero::point> points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};

/*        SECTION("using iterators") {
            fbuilder.add_points(points.cbegin(), points.cend());
        }

        SECTION("using iterators and size") {
            fbuilder.add_points(points.cbegin(), points.cend(), static_cast<uint32_t>(points.size()));
        }*/

        SECTION("using container directly") {
            fbuilder.add_points_from_container(points);
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();

    point_handler handler;
    feature.decode_point_geometry(handler);

    REQUIRE(handler.data == points);
}
/*
TEST_CASE("Add points from iterator with wrong count throws assert") {
    const std::vector<vtzero::point> points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.add_points(points.cbegin(),
                                          points.cend(),
                                          static_cast<uint32_t>(points.size() + 1)), const assert_error&);
}*/

