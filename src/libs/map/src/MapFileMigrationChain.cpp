#include <utility>

#include <antwika/schema/MigrationChain.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map::mapfile
{

    schema::MigrationChain getMapMigrations()
    {
        schema::MigrationList migrations;

        mapMigrationsV1To12(migrations);
        mapMigrationsV12To22(migrations);
        mapMigrationsV22To30(migrations);
        mapMigrationsV30To47(migrations);
        mapMigrationsV47To48(migrations);
        mapMigrationsV48To51(migrations);

        return schema::MigrationChain(
            std::move(migrations), kMapVersion);
    } // GCOVR_EXCL_LINE

}
