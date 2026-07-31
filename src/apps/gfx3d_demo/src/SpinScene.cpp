#include "antwika/gfx3d_demo/SpinScene.hpp"

#include <cstdint>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Transform.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Perspective;
    using antwika::gfx::Point;
    using antwika::gfx::Transform;
    using antwika::gfx::Vec3;

    namespace
    {

        /// How far the cube turns about its fastest axis in one tick.
        constexpr float kRadiansPerTick = 0.02F;

        /// The other two axes turn at these fractions of that.
        constexpr float kPitchShare = 0.7F;
        constexpr float kRollShare = 0.3F;

        /// Where the eye sits, and what it looks at.
        constexpr Vec3 kEye{1.6F, 1.4F, 2.6F};
        constexpr Vec3 kOrigin{0.0F, 0.0F, 0.0F};
        constexpr Vec3 kUp{0.0F, 1.0F, 0.0F};

        /// How the scene is flattened.
        constexpr float kFieldOfView = 0.9F;
        constexpr float kNearPlane = 0.1F;
        constexpr float kFarPlane = 20.0F;

        /// What is drawn behind the cube.
        constexpr Color kBackground{.red = 16, .green = 18, .blue = 28};

        /// The caption, and where and how big it goes.
        constexpr std::string_view kCaption = "antwika gfx3d demo";
        constexpr Point kCaptionOrigin{.x = 12, .y = 12};
        constexpr std::uint32_t kCaptionScale = 2;
        constexpr Color kCaptionColor{
            .red = 235, .green = 235, .blue = 245};

        /// What the cube's own colours are multiplied by.
        constexpr Color kNoTint{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

    } // namespace

    Mat4 SpinScene::modelAt(std::uint64_t tick) const
    {
        const auto turn = static_cast<float>(tick) * kRadiansPerTick;

        return Transform{
            .rotationRadians =
                {turn * kPitchShare, turn, turn * kRollShare}}
            .matrix();
    }

    Camera3D SpinScene::cameraFor(Size canvas) const
    {
        // A canvas with no height has no proportions.
        // Dividing by it puts a NaN in every matrix that follows.
        // Square is the answer that still draws something.
        const float aspect = canvas.height == 0
            ? 1.0F
            : static_cast<float>(canvas.width)
                / static_cast<float>(canvas.height);

        // Named rather than built inside the braces below.
        // A temporary there leaves gcov an unwind block on its line.
        const Perspective projection{
            .fovYRadians = kFieldOfView,
            .aspectRatio = aspect,
            .nearPlane = kNearPlane,
            .farPlane = kFarPlane};

        return Camera3D{kEye, kOrigin, kUp, projection};
    }

    void SpinScene::draw(
        IRenderer &flat,
        IRenderer3D &space,
        const IMesh &cube,
        Size canvas,
        std::uint64_t tick) const
    {
        flat.clear(kBackground);

        space.drawMesh(cube, modelAt(tick), cameraFor(canvas), kNoTint);

        flat.drawText(
            kCaptionOrigin, kCaption, kCaptionScale, kCaptionColor);
    }

} // namespace antwika::gfx3d_demo
