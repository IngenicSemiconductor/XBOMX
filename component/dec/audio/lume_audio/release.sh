#!/bin/bash
function gettop
{
    local TOPFILE=build/core/envsetup.mk
    if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
        echo $TOP
    else
        if [ -f $TOPFILE ] ; then
            # The following circumlocution (repeated below as well) ensures
            # that we record the true directory name and not one that is
            # faked up with symlink names.
            PWD= /bin/pwd
        else
            # We redirect cd to /dev/null in case it's aliased to
            # a command that prints something as a side-effect
            # (like pushd)
            local HERE=$PWD
            T=
            while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
                cd .. > /dev/null
                T=`PWD= /bin/pwd`
            done
            cd $HERE > /dev/null
            if [ -f "$T/$TOPFILE" ]; then
                echo $T
            fi
        fi
    fi
}
T=$(gettop)
arr=`grep "TARGET_PRODUCT:=" $T/buildspec.mk`
P=`echo ${arr#TARGET_PRODUCT:=}`
C=`pwd`
LPATH="$T/out/target/product/$P/obj/STATIC_LIBRARIES"
if [ -d $LPATH ];then
LIB="libstagefright_lumeaudiodec"
cd $C
rm -rf lib

for i in `echo $LIB`;do
echo $i
cp $LPATH/$i"_intermediates"/$i.a $C
done
cd -

RMPATH="LUMEAudioDecoder.cpp src include Android.static.mk release.sh"
cd $C
rm -rf $RMPATH 
sed -i 's/Android.static.mk/Android.lib.mk/g' Android.mk
cd -
else
echo "please check buildspec.mk!"
fi

