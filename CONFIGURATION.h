#pragma once

#define SERVERSEEKER_VERSION "v0.1"

// trust
#define DB_CONNECTION_URI "INSERT_POSTGRES_URI"

#define MASSCAN_ARGUMENTS "-p25565", "0.0.0.0/0", "--rate=100000000", "--exclude=255.255.255.255", "--ping"