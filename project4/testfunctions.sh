make clean
make


TESTIN="input.txt"
TESTOUT="output.txt"

ZEROIN="zeroin.txt"
ZEROOUT="zeroout.txt"

TESTIN_LARGE="large_input.txt"
TESTOUT_LARGE="large_output.txt"

DISK="test.img"
NONEXISTENT="noinode.txt"

echo "------------------------"
echo "testing all functions"
echo "------------------------"

# Clean up
rm -f $DISK $TESTOUT

# Run all functions  
./simplefs $DISK 50 <<EOF
help
format
mount
debug
create
create
getsize 0
getsize 1
getsize 2
copyin $TESTIN 0
copyout 0 $TESTOUT
debug
cat 0
delete 1
delete 999   
delete 40  
getsize 999  
getsize 40  
copyout 1 $NONEXISTENT
unmount
mount
debug
exit
EOF

# Check output
echo ""
echo "--- Output Comparison ---"
if cmp -s "$TESTIN" "$TESTOUT"; then
    echo "PASS: Files match!"
else
    echo "!!!!! FAIL: Files do not match!"
    diff "$TESTIN" "$TESTOUT"
fi

# Clean up files
rm -f  $TESTOUT $DISK $NONEXISTENT

echo "-------------------------"
echo "tesing all functions complete"
echo "-------------------------"


echo "------------------------"
echo "stress testing create"
echo "------------------------"

#   # Clean up                                                  XXXXXXXXXXXXXXXXXXXXXXXXXXX no working as entended 
rm -f $DISK 
# Run full test sequence
./simplefs $DISK 3 <<EOF
format
mount
debug
create
create
create
create
create
debug
unmount
mount
debug
EOF

rm -f $DISK

echo "-------------------------"
echo "stress testing create complete"
echo "-------------------------"

echo "-------------------------"
echo "test mounting and unmounting"
echo "-------------------------"




#   # Clean up                                              XXXXXXXXXXXXXXXXXXXXXXXXXXX might need to make debug not work when we arnt mounted 
rm -f $DISK 
# Run full test sequence
./simplefs $DISK 50 <<EOF
mount
format 
mount
create
create
unmount
debug
getsize 0
create
getsize 3
EOF

rm -f $DISK

echo "-------------------------"
echo "test mounting and unmounting complete"
echo "-------------------------"

echo "-------------------------"
echo "test invalid disk image"
echo "-------------------------"


INVALIDDISK="invalid.img"
# Make an invalid disk image
dd if=/dev/urandom of=$INVALIDDISK bs=4096 count=10 status=none

./simplefs $INVALIDDISK 10 <<EOF
mount
quit
EOF

echo "-------------------------"
echo "test invalid disk image"
echo "-------------------------"

echo "-------------------------"
echo "test zero lenght R/W"
echo "-------------------------"


ZEROIN="zeroin.txt"
ZEROOUT="zeroout.txt"

# Clean up
rm -f $DISK $ZEROOUT $ZEROIN

./simplefs $DISK 50 <<EOF
format
mount
create
copyin $ZEROIN 1
copyout 1 $ZEROOUT
getsize 1
quit
EOF

if [[ -s $ZEROOUT ]]; then
  echo "!!!!!!!!! Zero-length copyout produced data unexpectedly."
else
  echo "Zero-length read/write behaves correctly."
fi


echo "-------------------------"
echo "test zero lenght R/W complete"
echo "-------------------------"

echo "-------------------------"
echo "test large files"
echo "-------------------------"


# make sure disk and outfile are no still there 
rm -f $DISK $TESTOUT_LARGE


./simplefs $DISK 100 <<EOF
format
mount
debug
create                  
create                  
getsize 0
getsize 1
create                 
copyin $TESTIN_LARGE 2  # triggers indirect block
copyout 2 $TESTOUT_LARGE
getsize 2
debug
unmount
mount
debug
quit
EOF

echo ""
echo "--- Verifying Outputs ---"


if cmp -s "$TESTIN_LARGE" "$TESTOUT_LARGE"; then
    echo "Large file matches (indirect block handling works)."
else
    echo "Large file mismatch (problem with indirect blocks)."
    diff <(xxd "$TESTIN_LARGE") <(xxd "$TESTOUT_LARGE") | head -n 20
fi


echo "-------------------------"
echo "test large files complete"
echo "-------------------------"


echo "-------------------------"
echo "test large file that deosnt fit"
echo "-------------------------"
./simplefs $DISK 10 <<EOF
format
mount
debug          
# copyin $TESTIN_LARGE 2 
create
copyin $TESTIN_LARGE 0
copyout 0 $TESTOUT_LARGE
getsize 0
debug
EOF



# Clean up
rm -f  $TESTOUT $BIGFILE $ZEROOUT $INVALIDDISK $TESTOUT_LARGE $DISK inode_test_log.txt



