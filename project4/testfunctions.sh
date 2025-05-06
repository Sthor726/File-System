make clean
make

# # Remove existing disk image if it exists
# rm -f testdisk.img

# # Create a new disk with 50 blocks
# ./simplefs testdisk.img 50 <<EOF
# format
# mount
# debug
# create
# create
# getsize 0
# getsize 1
# copyin fs.c 0
# getsize 0
# copyout 0 output_fs.c
# delete 0
# debug
# unmount
# quit
# EOF

# echo "Test complete."






# DISK_IMG="testrw.img"
# BLOCKS=100
# TESTFILE="testinput.txt"
# OUTFILE="testoutput.txt"

# # Create test input file
# echo "There is something to be said about saying something, wouldnt ya say" > $TESTFILE
# echo "1234567890" >> $TESTFILE
# head -c 8000 /dev/urandom >> $TESTFILE  # Add 8KB of random data to test indirect

# # Clean up
# rm -f $DISK_IMG $OUTFILE

# # Run tests
# ./simplefs $DISK_IMG $BLOCKS <<EOF
# format
# mount
# create
# create
# copyin $TESTFILE 0
# copyout 0 $OUTFILE
# getsize 0
# getsize 1
# delete 1
# getsize 1
# copyout 2 testfail.txt
# delete 999
# unmount
# quit
# EOF


# echo ""
# if cmp -s "$TESTFILE" "$OUTFILE"; then
#     echo "✅ Read/write passed: input and output files match."
# else
#     echo "❌ ERROR: Files differ! Check output."
#     diff <(xxd "$TESTFILE") <(xxd "$OUTFILE") | head -n 20
# fi

# # Clean up temporary files
# #rm -f $TESTFILE $OUTFILE






DISK="testall.img"

TESTIN="input.txt"
TESTOUT="output.txt"
NONEXISTENT="noinode.txt"


# Clean up
rm -f $DISK $TESTOUT

# Run full test sequence
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
    echo "✅ PASS: Files match!"
else
    echo "❌ FAIL: Files do not match!"
    diff "$TESTIN" "$TESTOUT"
fi

# Clean up files
rm -f  $TESTOUT $DISK $NONEXISTENT