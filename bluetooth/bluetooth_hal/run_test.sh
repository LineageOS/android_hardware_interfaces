#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Check for help flags
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  atest "$@"
  exit $? # Exit with atest's exit code
fi

# Parse command-line arguments
args=()
host_filter=false
while [[ $# -gt 0 ]]; do
  if [[ "$1" == "--host" ]]; then
    host_filter=true
  fi
  args+=("$1")
  shift
done

android_bp_file="$(dirname "$0")/Android.bp"
test_info=() # Array to store "name:host_supported" or "name:"

# Use awk to extract test names and host_supported status
# This awk script finds cc_test blocks, captures the name,
# and checks for host_supported: false within the same block.
# It outputs "name:true" if host_supported is true or missing,
# and "name:false" if host_supported is false.
readarray -t test_info < <(awk '
/^cc_test \{/ {
  in_test_block=1;
  current_test_name="";
  host_supported=1; # Assume true by default if not specified false
  device_supported=1; # Assume true by default if not specified false
}
in_test_block {
  if ($1 == "name:") {
    match($0, /name: "([^"]+)"/, a);
    if (a[1]) current_test_name=a[1];
  }
  # Check for explicit false
  if ($1 == "host_supported:" && $2 == "false,") {
    host_supported=0;
  }
  if ($1 == "device_supported:" && $2 == "false,") {
    device_supported=0;
  }
  if ($1 == "}") {
    if (current_test_name != "") {
      print current_test_name ":" (host_supported ? "true" : "false") ":" (device_supported ? "true" : "false")
    }
    in_test_block=0;
    current_test_name="";
    host_supported=1; # Reset for next block
    device_supported=1; # Reset for next block
  }
}
' "$android_bp_file")

test_count=${#test_info[@]}
tests_to_run=()
skipped_tests=() # Store name and reason for skipping

# Modified filtering logic based on host_filter, host_supported, and device_supported
if [[ "$test_count" -gt 0 ]]; then
  echo ""
  echo "====================================================="
  echo "Found $test_count test cases:"
  for info in "${test_info[@]}"; do
    IFS=':' read -r name host_supported_status device_supported_status <<< "$info"
    echo "  - $name" # Still list all found tests initially

    skip_reason=""
    should_skip=false

    if [[ "$host_filter" == true && "$host_supported_status" == "false" ]]; then
      should_skip=true
      skip_reason="[host_supported: false] with [--host]"
    elif [[ "$host_filter" == false && "$device_supported_status" == "false" ]]; then
      should_skip=true
      skip_reason="[device_supported: false] without [--host]"
    fi

    if [[ "$should_skip" == true ]]; then
      skipped_tests+=("$name ($skip_reason)") # Store name and reason
    else
      tests_to_run+=("$name")
    fi
  done
  echo "====================================================="
  echo ""

  # Modified skipped message printing to include reason
  if [[ "${#skipped_tests[@]}" -gt 0 ]]; then
      echo -e "${YELLOW}Skipping tests:"
      for skipped_test_info in "${skipped_tests[@]}"; do
          echo "  - $skipped_test_info"
      done
      echo -e "${NC}"
  fi

  if [[ "${#tests_to_run[@]}" -eq 0 ]]; then
      echo "No tests to run after filtering."
      # Print skipped tests here too if any were skipped
      if [[ "${#skipped_tests[@]}" -gt 0 ]]; then
          echo "Skipped tests:"
          for skipped_test_info in "${skipped_tests[@]}"; do
              echo "  - $skipped_test_info"
          done
      fi
      exit 0
  fi

  # Build the atest command
  test_command="atest"
  for test_name in "${tests_to_run[@]}"; do
    test_command+=" $test_name"
  done
  test_command+=" ${args[@]}"

  echo ""
  echo "Running test command:"
  echo "  $test_command"
  echo ""

  # Run the combined atest command
  eval "$test_command"
  atest_exit_code=$?

  # Print skipped tests after complete
  if [[ "${#skipped_tests[@]}" -gt 0 ]]; then
      echo -e "${YELLOW}"
      echo "Skipped tests:"
      for skipped_test_info in "${skipped_tests[@]}"; do
          echo "  - $skipped_test_info"
      done
      echo ""
  fi

  if [[ $atest_exit_code -ne 0 ]]; then
    echo -e "${RED}"
    echo "███████╗ █████╗ ██╗██╗     "
    echo "██╔════╝██╔══██╗██║██║     "
    echo "█████╗  ███████║██║██║     "
    echo "██╔══╝  ██╔══██║██║██║     "
    echo "██║     ██║  ██║██║███████╗"
    echo "╚═╝     ╚═╝  ╚═╝╚═╝╚══════╝"
    echo "                           "
    echo "One or more tests failed!"
    exit 1
  else
    echo -e "${GREEN}"
    echo -e "██████╗  █████╗ ███████╗███████╗██╗"
    echo -e "██╔══██╗██╔══██╗██╔════╝██╔════╝██║"
    echo -e "██████╔╝███████║███████╗███████╗██║"
    echo -e "██╔═══╝ ██╔══██║╚════██║╚════██║╚═╝"
    echo -e "██║     ██║  ██║███████║███████║██╗"
    echo -e "╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝"
    echo "                                      "
    echo -e "All tests completed successfully!${NC}"
  fi
else
  echo "No matching cc_test names found in Android.bp."
  exit 1
fi

exit 0
