#!/bin/sh

# Script constants
I2C_BUS=1
ADDR_BASE=0x50
MAX_BANK=1
MAX_BANK_SIZE=255
LOG_PATH=/mnt/data/eeprom.log

# Global variables
VERBOSE=0
TEST=0

usage()
{
    SCRIPT=$(basename $0)
    echo "$SCRIPT -h -r|-w -f <file>|-b [<value>] -ba <bank> -o <value> [-n] [-s]"
    echo ""
    echo "-h        This usage information"
    echo "-s        Explicit Serial Info (Use bytes 0 through 7)"
    echo "-r        Read value"
    echo "-w        Write value"
    echo "-f        File input/output"
    echo "-by       Byte input/output"
    echo "-o        EEPROM offset, valid values 8 - 255 (0 through 7 must use explicit -s)"
    echo "-ba       EEPROM bank, valid values 0 - $MAX_BANK"
    echo "-n        Add null term after byte write"
    echo "-t        Test on non-embedded hardware"
    echo "-v        Verbose output"
    echo "NOTE: All data must be in hex without a preceeding \"0x\" and all others should be in decimal"
}

output_verbose()
{
    if [ $VERBOSE -ne 0 ]; then
        echo $1
    fi
}

output_to_log()
{
    MSG="$1"
    echo "$MSG" >> $LOG_PATH
}

finalize_address()
{
    MOD=$1
    ADDR=$((ADDR_BASE + MOD))
    printf "%x" $ADDR
}

format_hex()
{
    VALUE=$1
    echo $(printf "0x%02x" $VALUE)
}

get_file_value()
{
    FILE=$1
    INDEX=$2
    echo $(hexdump -n 1 -s $INDEX -e '/1 "%02x"' $FILE)
}

read_byte()
{
    # Prep values for doing data
    BUS=$1
    ADDR="0x$2"
    OFFSET=$(format_hex $3)

    output_to_log "READ- Bus: $BUS, Addr: $ADDR, Offset: $OFFSET"

    # Action based on testing
    if [ $TEST -eq 0 ]; then
        RESULT=$(i2cget -y $BUS $ADDR $OFFSET | cut -dx -f2)
    else
        RESULT=41
    fi
    echo $RESULT
}

write_byte()
{
    # Prep values for doing data
    BUS=$1
    ADDR="0x$2"
    OFFSET=$(format_hex $3)
    VALUE="0x$4"
    
    output_to_log "WRITE- Bus: $BUS, Addr: $ADDR, Offset: $OFFSET, Value: $VALUE"

    # Action based on testing
    if [ $TEST -eq 0 ]; then
        RESULT=$(i2cset -y $BUS $ADDR $OFFSET $VALUE)
    fi
}

# Validate the options were passed
if [ $# -eq 0 ]; then
    echo "No options provided."
    usage
    exit 1
fi

# Handle all options provided
while [ $# -gt 0 ]; do
    case $1 in
        -h)
            usage
            exit 0
            ;;
        -v)
            VERBOSE=1
            shift
            ;;
        -t)
            TEST=1
            LOG_PATH="./eeprom.log"
            shift
            ;;
        -r)
            READ="TRUE"
            shift
            ;;
        -w)
            WRITE="TRUE"
            shift
            ;;
        -f)
            FILE=$2
            shift
            shift
            ;;
        -ba)
            BANK=$2
            shift
            shift
            ;;
        -by)
            if [ "$WRITE" = "TRUE" ]; then
                BYTE=$2
                shift
            fi
            
            USE_BYTE="TRUE"
            shift
            ;;
        -o)
            OFFSET=$2
            shift
            shift
            ;;
        -n)
            NULL_TERM="TRUE"
            shift
            ;;
        -s)
            SERIAL_INFO="TRUE"
            shift
            ;;
        -*|--*)
            echo "Unknown option $1."
            usage
            exit 1
            ;;
        *)
            echo "Unknown value $1."
            usage
            exit 1
            shift
            ;;
    esac
done

# Start verbose output
output_verbose "$(basename $0):"
output_verbose ""
output_verbose "Parced Arguments:"
output_verbose "Read       = $READ"
output_verbose "Write      = $WRITE"
output_verbose "File       = $FILE"
output_verbose "Byte       = $BYTE"
output_verbose "Bank       = $BANK"
output_verbose "Offset     = $OFFSET"
output_verbose "Null Term  = $NULL_TERM"
output_verbose "SerialInfo = $SERIAL_INFO"

# Validate that the read or write operators were provided
if [ "$READ" = "TRUE" ] && [ "$WRITE" = "TRUE" ]; then
    echo "Both read and write options were indicated which is invalid"
    exit 1
elif [ "$READ" = "" ] && [ "$WRITE" = "" ]; then
    echo "Providing the read or write option is required"
    exit 1
fi

# Validate that the file or byte operators were provided
if [ "$FILE" != "" ] && [ "$USE_BYTE" = "TRUE" ]; then
    echo "Both file and byte operations were indicated which is invalid"
    exit 1
elif [ "$FILE" == "" ] && [ "$USE_BYTE" == "" ]; then
    echo "Providing the file or byte input/output option is required"
    exit 1
elif [ ! -f $FILE ] && [ "$WRITE" = "TRUE" ]; then
    echo "Unable to read file that doesn't exist"
    exit 1
fi

# Validate bank
if [ "$BANK" = "" ]; then
    echo "An address bank is required"
    exit 1
elif [ $BANK -gt $MAX_BANK ]; then
    echo "An address bank has a maximum of $MAX_BANK"
    exit 1
fi

# Validate offset (and shorten MAX_BANK_SIZE if serial)
if [ "$OFFSET" = "" ]; then
    echo "An address offset is required"
    exit 1
elif [ $OFFSET -lt 32 ]; then
  if [ "$SERIAL_INFO" = "TRUE" ]; then
    MAX_BANK_SIZE=32
  else
    echo "Attempting to read/write within serial info range"
    exit 1
  fi
elif [ $OFFSET -gt 31 ]; then
  if [ "$SERIAL_INFO" = "TRUE" ]; then
    echo "Offset is outside valid serial info range"
    exit 1
  elif [ $OFFSET -gt 255 ]; then
    echo "Offset is outside valid range"
    exit 1
  fi
fi

# Determine how long we should loop
# If doing byte, interaction count is 1
if [ "$USE_BYTE" = "TRUE" ]; then
    COUNT=1
# If doing file write, determine the count
elif [ "$FILE" != "" ] && [ "$WRITE" = "TRUE" ]; then
    COUNT=$(wc -c $FILE | cut -d" " -f1)
# If doing file read, we will loop until a null or the end of the block
else
    COUNT=-1
fi

if [ $COUNT -ne -1 ] && [ $((OFFSET + COUNT)) -gt $MAX_BANK_SIZE ]; then
    echo "Offset and size larget than bank allowable size"
    exit 1
fi

# Setup values to be used later
ACTION_ADDR=$(finalize_address $BANK)
INDEX=0
CURR_OFFSET=$OFFSET

# Loop through until the index is less than the count or until we break if we
# don't know the count
while [ $INDEX -lt $COUNT ] || [ $COUNT -eq -1 ]; do
    # If reading
    if [ "$READ" = "TRUE" ]; then
        # Read the value and format it
        VALUE=$(read_byte $I2C_BUS $ACTION_ADDR $CURR_OFFSET)
        OUTPUT_VALUE=$(echo $VALUE | xxd -r -p)

        # If we're looping infinitely determine if we need to break
        if [ $COUNT -eq -1 ]; then
            # If the read value was a null char
            if [ "$VALUE" = "00" ]; then
                output_verbose "Null character found"
                break
            # If we don't have any more room to read in this bank
            elif [ $CURR_OFFSET -eq $MAX_BANK_SIZE ]; then
                output_verbose "Stopped at last valid offset"
                break
            fi
        fi
        
        # Direct the read value based on the input options
        if [ "$USE_BYTE" = "TRUE" ]; then
            echo $OUTPUT_VALUE
        else
            echo -n $OUTPUT_VALUE >> $FILE
        fi
    # If writing
    elif [ "$WRITE" = "TRUE" ]; then
        # If we're only writing a byte
        if [ "$USE_BYTE" = "TRUE" ]; then
            # Write the byte
            write_byte $I2C_BUS $ACTION_ADDR $CURR_OFFSET $BYTE

            # If the null term option was passed write it
            if [ "$NULL_TERM" = "TRUE" ]; then
                NULL_OFFSET=$((CURR_OFFSET + 1))
                write_byte $I2C_BUS $ACTION_ADDR $NULL_OFFSET 00
            fi
        # Otherwise we're trying to write from a file
        else
            # Get the current index value from the file and write it to EEPROM
            VALUE=$(get_file_value $FILE $INDEX)
            write_byte $I2C_BUS $ACTION_ADDR $CURR_OFFSET $VALUE

            # If we just wrote the last value, also write the null terminator
            if [ $INDEX -eq $((COUNT - 1)) ]; then
                NULL_OFFSET=$((CURR_OFFSET + 1))
                write_byte $I2C_BUS $ACTION_ADDR $NULL_OFFSET 00
            fi
        fi
    fi

    # Increment values for the next pass
    INDEX=$((INDEX + 1))
    CURR_OFFSET=$((CURR_OFFSET + 1))
done
