#pragma once
// ----------------------------------------------------------------------------
// Test identity values for the OCR / parser suites.
//
// Defaults are FULLY FICTIONAL, so the tests pass with no setup. To validate
// parsing against a REAL card locally, set these in a gitignored .env (see
// .env.example) and export them before running:  set -a; . ./.env; set +a
// Real driver's-license data must NEVER be committed to this repository.
// ----------------------------------------------------------------------------
#include <cstdlib>
#include <string>

namespace dlscan {
namespace test {

inline std::string env_or(const char* key, const char* fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::string(v) : std::string(fallback);
}

// Date of birth — as printed on the card (US MM/DD/YYYY) and ISO-normalized.
inline std::string kDobSlash() { return env_or("DLSCAN_TEST_DOB_SLASH", "01/02/1990"); }
inline std::string kDobIso()   { return env_or("DLSCAN_TEST_DOB_ISO",   "1990-01-02"); }

// Wisconsin DLN shape: 1 letter + 13 digits, printed "L###-####-####-##".
// kDlnFull is the clean whole-card read; kDlnClipped is the 3-4-4 fragment a
// per-region crop can collapse it to (dropping the leading letter + last group).
inline std::string kDlnFull()    { return env_or("DLSCAN_TEST_DLN_FULL",    "P135-2468-1357-09"); }
inline std::string kDlnClipped() { return env_or("DLSCAN_TEST_DLN_CLIPPED", "135-2468-1357"); }

}  // namespace test
}  // namespace dlscan
