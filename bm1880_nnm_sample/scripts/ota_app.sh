#!/bin/sh

echo "do app upgrade!!!"

extract()
{
    echo move download file to /tmp
    echo "0" > /tmp/upgrade_status

    mv /bin/download.bin* /tmp/.

    echo compose to one file

    cat /tmp/download.bin* > /tmp/ota.zip

    echo unzip begin

    #if [ ! -f /tmp/download.bin0 ]
    #then
    #    cat update.zip.* > update.zip
    #fi

    #rm -rf update
    #unzip update.zip

    unzip /tmp/ota.zip -d /tmp/.

#    /bin/rm update.zip*
    echo Unzip done
}

movefiles()
{
    echo Move files

    if [ -d /tmp/rootfs ]
    then
	cp -a /tmp/rootfs/* /.
    fi

    echo "1" > /tmp/upgrade_status

    rm -rf /tmp/rootfs

    echo move files done
}

extract
movefiles

