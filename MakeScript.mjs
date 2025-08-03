// RUN npm init -y
// RUN npm install bitmake
// RUN npx bitmake build

export default (mk) => {
  mk.PROJECT_NAME = "dbc";
  mk.PROJECT_VERSION = "0.0.1";
  mk.PROJECT_DESCRIPTION = "DBC handles parsing and encoding CAN data using DBC files for automotive systems";
  mk.PROJECT_HOMEPAGE_URL = "https://github.com/yacubin/dbc";

  mk.DBC_CPU_32BIT = (mk.SIZEOF_VOID_P == 4);
  mk.DBC_LOG_ENABLE = (mk.BUILD_TYPE === "Debug");

  const headers = [
    "dbc/DBCAlloc.h",
    "dbc/DBCAssert.h",
    "dbc/DBCDocBuilder.h",
    "dbc/DBCDocument.h",
    "dbc/DBCDocWriter.h",
    "dbc/DBCParser.h",
    "dbc/DBCID.h",
    "dbc/DBCJ1939.h",
    "dbc/DBCLog.h",
    "dbc/DBCKeywords.h",
    "dbc/DBCErrorCode.h",
    "dbc/DBCTypes.h",
  ];

  const sources = [
    "dbc/DBCAlloc.c",
    "dbc/DBCDocBuilder.c",
    "dbc/DBCDocument.c",
    "dbc/DBCDocWriter.c",
    "dbc/DBCParser.c",
    "dbc/DBCID.c",
    "dbc/DBCJ1939.c",
    "dbc/DBCLog.c",
    "dbc/DBCTypes.c",
  ];

  const includes = [
    mk.BINARY_DIR,
    mk.SOURCE_DIR,
  ];

  const libraries = [
  ];

  const config_h = mk.BINARY_DIR.join("config.h");
  mk.addCustomScript("configure_file", {
    SCRIPT_INPUT: "config.h.cmake",
    SCRIPT_OUTPUT: config_h,
  });

  const dbc = mk.addStaticLibrary("dbc", sources, config_h);
  dbc.addPublicIncludes(includes);
  dbc.addPublicLibraries(libraries);
  dbc.setPositionIndependentCode(true);

  mk.install(headers, "include");
  mk.install(dbc, "lib");
}
