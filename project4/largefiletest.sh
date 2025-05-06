make clean
make

DISK="testlargefile.img"

TESTIN_LARGE="large_input.txt"
TESTOUT_LARGE="large_output.txt"


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
    echo "✅ Large file matches (indirect block handling works)."
else
    echo "❌ Large file mismatch (problem with indirect blocks)."
    diff <(xxd "$TESTIN_LARGE") <(xxd "$TESTOUT_LARGE") | head -n 20
fi


# clean up after
rm -f $TESTOUT_LARGE $DISK 


