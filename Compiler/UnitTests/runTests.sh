export PATH=$PATH:/usr/local/boost/boost_1_87_0/lib/

args="$*"

# Boost test args: edit here to add your own
boostArgs="--log_level=test_suite $args"
executable=./Bin/UnitTests

echo "Running tests with boost args: $boostArgs"
echo ------------------------------------------------------

$executable $boostArgs