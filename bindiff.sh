#!/usr/bin/env bash

source $1

TOTAL_SYMS=0
EQUAL_SYMS=0
VERBOSE=0

has_sym() {
  local SYM=$1

  if nm --defined-only --just-symbols $TARGET_BIN | grep -q -x $SYM; then
    return 0
  fi

  if nm --defined-only --just-symbols $BUILD_BIN | grep -q -x $SYM; then
    return 0
  fi

  return 1
}

get_syms() {
  local BEGIN=$1
  local END=$2
  local SYMS=()

  while IFS= read -r LINE; do
    ENTRY=($LINE)

    if [[ "0x${ENTRY[0]}" -ge "0x$BEGIN" ]] && [[ "0x${ENTRY[0]}" -lt "0x$END" ]]; then
      SYMS+=(${ENTRY[2]})
    fi
  done < <(nm --defined-only --numeric-sort $TARGET_BIN)

  echo ${SYMS[*]}
}

diff_sym() {
  local SYM=$1

  # FIXME make this handle data symbols

  objdump --no-addresses --no-show-raw-insn --disassemble=$SYM $TARGET_BIN | grep -v "file format\|Disassembly of section\|^$" >/tmp/target_asm
  objdump --no-addresses --no-show-raw-insn --disassemble=$SYM $BUILD_BIN | grep -v "file format\|Disassembly of section\|^$" >/tmp/build_asm

  local PADDING=$(printf %40s)
  local PADDED=$SYM$PADDING
  local TARGET_CRC="MISSING"
  local BUILD_CRC="MISSING"

  if [ -s /tmp/target_asm ]; then
    TARGET_CRC=$(cat /tmp/target_asm | md5sum | cut -d ' ' -f 1)
  fi

  if [ -s /tmp/build_asm ]; then
    BUILD_CRC=$(cat /tmp/build_asm | md5sum | cut -d ' ' -f 1)
  fi


  TOTAL_SYMS=$((TOTAL_SYMS + 1))

  if [[ "$TARGET_CRC" == "MISSING" ]] && [[ "$BUILD_CRC" == "MISSING" ]]; then
    echo -e "\e[31m\u274c ${PADDED:0:40}\e[0m \e[33mmissing from both\e[0m"
  elif [[ "$TARGET_CRC" == "MISSING" ]]; then
    echo -e "\e[31m\u274c ${PADDED:0:40}\e[0m \e[33mmissing from target\e[0m"
  elif [[ "$BUILD_CRC" == "MISSING" ]]; then
    echo -e "\e[31m\u274c ${PADDED:0:40}\e[0m \e[33mmissing from build\e[0m"
  elif [[ "$TARGET_CRC" != "$BUILD_CRC" ]]; then
    echo -e "\e[31m\u274c ${PADDED:0:40}\e[0m $BUILD_CRC"

    if [ $VERBOSE -eq 1 ]; then
      diff --old-line-format=$'\e[0;33m< %L\e[0m'      \
           --new-line-format=$'\e[0;31m> %L\e[0m'      \
           --unchanged-group-format=$'\e[0;32m%=\e[0m' \
           /tmp/target_asm /tmp/build_asm
    fi
  else
    EQUAL_SYMS=$((EQUAL_SYMS + 1))

    echo -e "\e[32m\u2713  ${PADDED:0:40}\e[0m $BUILD_CRC" >&2
  fi
}

diff_data() {
  local OBJNAME=$1
  local BEGIN=$2
  local END=$3

  objdump -s --start-address=0x$BEGIN --stop-address=0x$END $TARGET_BIN | grep -v "file format\|Contents of section\|^$" >/tmp/target_data
  objdump -s --start-address=0x$BEGIN --stop-address=0x$END $BUILD_BIN | grep -v "file format\|Contents of section\|^$" >/tmp/build_data

  if ! cmp --silent /tmp/target_data /tmp/build_data; then
    echo -e "\e[31m\u274c $OBJNAME [$BEGIN,$END)\e[0m"
    icdiff /tmp/target_data /tmp/build_data
  fi
}

diff_code() {
  local OBJNAME=$1
  local BEGIN=$2
  local END=$3
  local SYMS=$(get_syms $BEGIN $END)

  for SYM in ${SYMS[@]}; do
    # ignore labels
    if [[ $SYM == .L* ]]; then
      continue
    fi

    diff_sym $SYM
  done
}

diff_section() {
  local SECTION=$1
  local FILTER=$2

  declare -n ARR=$SECTION

  echo "diffing $SECTION section..."

  TOTAL_SYMS=0
  EQUAL_SYMS=0

  for i in ${!ARR[@]}; do
    ENTRY=(${ARR[$i]})

    OBJNAME=${ENTRY[0]}
    BEGIN=${ENTRY[1]}
    END=${ENTRY[2]}

    if [[ ! -z $FILTER ]] && [[ $OBJNAME != $FILTER ]]; then
      continue
    fi

    if [[ "0x$BEGIN" -eq "0x$END" ]]; then
      continue
    fi

    if [[ "$SECTION" == "TEXT" ]]; then
      diff_code $OBJNAME $BEGIN $END
    elif [[ "$SECTION" == "RODATA" ]] || [[ "$SECTION" == "DATA" ]]; then
      diff_data $OBJNAME $BEGIN $END
    fi
  done

  if [ "$SECTION" == "TEXT" ]; then
    echo "$EQUAL_SYMS/$TOTAL_SYMS symbols matched"
  fi
}

if [ ! -z "$2" ] && has_sym $2; then
  VERBOSE=1
  diff_sym $2
elif [ ! -z "$2" ] && [[ " ${SECTIONS[*]} " =~ [[:space:]]${2}[[:space:]] ]]; then
  diff_section $2 $3
else
  for SECTION in ${SECTIONS[@]}; do
    diff_section $SECTION $2
  done
fi
