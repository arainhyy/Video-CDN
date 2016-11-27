#!/bin/bash

ANDREW_USERNAME=$1

HOME=/home/project3
USERNAME=project3

RPMFUSION_FREE_RPM="http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm"

PACKAGES=(gcc-c++ git curl vim tmux kernel-modules-extra tar python-matplotlib kmod-VirtualBox VirtualBox-guest)

STARTER_REPO="https://github.com/letitz/bitrate-project-starter.git"
STARTER_REPO_DIR="bitrate-project-starter"

APACHE_DOWNLOAD="http://mirrors.sonic.net/apache//httpd/httpd-2.2.29.tar.gz"
APACHE_TARBALL="httpd-2.2.29.tar.gz"
APACHE_SRC_DIR="httpd-2.2.29"

CLICK_DOWNLOAD="http://www.read.cs.ucla.edu/click/click-2.0.1.tar.gz"
CLICK_TARBALL="click-2.0.1.tar.gz"
CLICK_SRC_DIR="click-2.0.1"

AFS_DIR="/afs/andrew.cmu.edu/usr/trigoudy/15441-p3"

WWW_TARBALL="www.tar.gz"
WWW_SRC_DIR="www"

F4F_TARBALL="adobe_f4f_apache_module_4_5_1_linux_x64.tar.gz"
F4F_SRC_DIR="adobe_f4f_apache_module_4_5_1_linux_x64"

F4F_CONF="LoadModule f4fhttp_module modules/mod_f4fhttp.so\n
<IfModule f4fhttp_module>\n
<Location /vod>\n
HttpStreamingEnabled true\n
HttpStreamingContentPath \"/var/www/vod\"\n
</Location>\n
</IfModule>\n"

TC=/usr/sbin/tc
CLICK=/usr/local/bin/click
APACHE=/usr/local/apache2/bin/httpd
APACHE_CONF_DIR=/usr/local/apache2/conf
APACHE_MODULES_DIR=/usr/local/apache2/modules

extract_tarball() {
    cd $HOME
    if [ ! -d $2 ]; then
        echo "Extracting $2..."
        tar -xf $1
    fi
}

download_tarball() {
    cd $HOME
    if [ ! -f $2 ]; then
        echo "Downloading $2..."
        curl -# $1 -o $2
    fi
}

scp_tarball() {
    cd $HOME
    if [ ! -f $1 ]; then
        echo "Scping $1..."
        scp "$ANDREW_USERNAME@unix.andrew.cmu.edu:$AFS_DIR/$1" ./
    fi
}

install_tarball() {
    download_tarball $1 $2
    extract_tarball $2 $3
    echo "Installing $3..."
	cd $3
    if [ ! -f already_configured ]; then
        echo "Configuring..."
    	./configure >/dev/null && touch already_configured
    fi
    echo "Building..."
	make >/dev/null
    echo "Installing files..."
	make install >/dev/null
}

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [ -z "$1" ]; then
    echo "USAGE: $0 <YOUR_ANDREW_USERNAME>"
    exit 1
fi

# Make sure user's home dir exists where we think it does
if [ ! -d "$HOME" ]; then
	echo "Could not find home directory: $HOME"
	exit 1
fi

# Enable RPMFusion free repo for virtualbox guest additions
echo "Enabling RPMFusion free repository..."
yum localinstall -y --nogpgcheck $RPMFUSION_FREE_RPM >/dev/null

echo "Updating packages, this may take a while..."
yum -y update >/dev/null

# Install packages
echo "Installing packages..."
yum install -y ${PACKAGES[*]} >/dev/null

# Removing fedora firewall
echo "Removing fedora firewall..."
yum remove -y firewalld >/dev/null
echo

# Install Click 2.0.1
install_tarball $CLICK_DOWNLOAD $CLICK_TARBALL $CLICK_SRC_DIR
echo

# Install Apache 2.2.5
install_tarball $APACHE_DOWNLOAD $APACHE_TARBALL $APACHE_SRC_DIR
echo

## Update Firefox
#echo "Updating Firefox..."
#yum update firefox
#
## Install flash plugin
#echo "Installing Flash plugin..."
#yum install http://linuxdownload.adobe.com/adobe-release/adobe-release-x86_64-1.0-1.noarch.rpm -y  # 64-bit
##yum install http://linuxdownload.adobe.com/adobe-release/adobe-release-i386-1.0-1.noarch.rpm -y   # 32-bit
#rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-adobe-linux
#yum install flash-plugin -y

# Install Adobe f4f origin module for apache
echo "Installing Adobe f4f origin module for apache..."
scp_tarball $F4F_TARBALL
extract_tarball $F4F_TARBALL $F4F_SRC_DIR
cp ./$F4F_SRC_DIR/* /$APACHE_MODULES_DIR
if ! grep -q "f4f" $APACHE_CONF_DIR/httpd.conf
then
    echo -e $F4F_CONF >> $APACHE_CONF_DIR/httpd.conf
fi
echo

# Copy www files to /var/www
echo "Installing www files..."
scp_tarball $WWW_TARBALL
extract_tarball $WWW_TARBALL $WWW_SRC_DIR
rm -rf /var/www
mv $WWW_SRC_DIR /var/www
echo

# Set permissions
echo "Setting file permissions..."
chmod 6755 $TC
chmod 6755 $CLICK
chmod 6755 $APACHE
chmod 777 $APACHE_CONF_DIR
chmod 777 $APACHE_CONF_DIR/httpd.conf

# Clone starter code
cd $HOME
if [ ! -d $STARTER_REPO_DIR ]; then
    echo "Cloning starter code..."
    sudo -u $USERNAME git clone $STARTER_REPO >/dev/null
else
    echo "Pulling last starter code..."
    cd $STARTER_REPO_DIR
    git pull >/dev/null
fi

echo "Done."
echo -n "Rebooting is recommended. Do you wish to reboot now? [Y/n]"
read do_reboot
if [ "$do_reboot" == "Y" ]; then
    echo "Rebooting..."
    reboot
fi
