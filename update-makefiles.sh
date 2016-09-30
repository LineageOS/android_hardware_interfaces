#!/bin/bash

if [ ! -d hardware/interfaces ] ; then
  echo "Where is hardware/interfaces?";
  exit 1;
fi

packages=$(pushd hardware/interfaces > /dev/null; \
           find . -type f -name \*.hal -exec dirname {} \; | sort -u | \
           cut -c3- | \
           awk -F'/' \
                '{printf("android.hardware"); for(i=1;i<NF;i++){printf(".%s", $i);}; printf("@%s\n", $NF);}'; \
           popd > /dev/null)

for p in $packages; do
  echo "Updating $p";
  hidl-gen -Lmakefile -r android.hardware:hardware/interfaces $p;
  hidl-gen -Landroidbp -r android.hardware:hardware/interfaces $p;
done
