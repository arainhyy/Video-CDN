import sys
sys.path.append('../common')

import os
import shutil
import logging
import platform
from util import check_output, strip_comments

NETSIM_STRING = '# Modified by netsim'

APACHE_UBUNTU = '/etc/init.d/apache2'
APACHE_UBUNTU_PORTS = '/etc/apache2/ports.conf'
APACHE_UBUNTU_PORTS_BAK = '%s.backup' % APACHE_UBUNTU_PORTS
APACHE_UBUNTU_DEFAULT_SITE = '/etc/apache2/sites-available/default'
APACHE_UBUNTU_SITES_AVAILABLE = '/etc/apache2/sites-available'
APACHE_UBUNTU_SITES_ENABLED = '/etc/apache2/sites-enabled'

APACHE_RHEL = '/usr/sbin/httpd'
APACHE_RHEL_CONF = '/etc/httpd/conf/httpd.conf'
APACHE_RHEL_CONF_BAK = '/etc/httpd/conf/httpd.conf.bak'
APACHE_RHEL_DOC_ROOT = '/var/www/html'

APACHE_FEDORA = '/usr/local/apache2/bin/httpd'
APACHE_FEDORA_CONF = '/usr/local/apache2/conf/httpd.conf'
APACHE_FEDORA_CONF_BAK = '/usr/local/apache2/conf/httpd.conf.bak'
APACHE_FEDORA_DOC_ROOT = '/var/www'

APACHE_VIRTUAL_HOST_TEMPLATE = '''

Listen %s:8080
<VirtualHost %s:8080>
    ServerAdmin webmaster@localhost
    ServerName video.cs.cmu.edu:8080

    DocumentRoot %s
    <Directory />
        Options FollowSymLinks
        AllowOverride None
    </Directory>
    <Directory %s/>
        Options Indexes FollowSymLinks MultiViews
        AllowOverride None
        Order allow,deny
        allow from all
    </Directory>

</VirtualHost>'''

LINUX = platform.linux_distribution()[0]

def is_apache_configured_split_conf(ports):
    found = False
    try:
        with open(ports, 'r') as portsf:
            for line in portsf:
                if NETSIM_STRING in line:
                    found = True
                    break
        portsf.closed
    except Exception as e:
        logging.getLogger(__name__).error(e)
    return found

def is_apache_configured_single_conf(conf):
    found = False
    try:
        with open(conf, 'r') as conff:
            for line in conff:
                if NETSIM_STRING in line:
                    found = True
                    break
        conff.closed
    except Exception as e:
        logging.getLogger(__name__).error(e)
    return found

def is_apache_configured():
    if  LINUX == 'Ubuntu':
        return is_apache_configured_split_conf(APACHE_UBUNTU_PORTS)
    elif LINUX == 'Fedora':
        return is_apache_configured_single_conf(APACHE_FEDORA_CONF)
    else:
        return is_apache_configured_single_conf(APACHE_RHEL_CONF)


def configure_apache_single_conf(ip_list, conf, conf_bak, doc_root):
    try:
        # back up the existing httpd.conf
        shutil.copyfile(conf, conf_bak)

        found = False
        with open(conf, 'r') as conffile:
            for line in conffile:
                if 'ServerName' in line and line[0] != '#':
                    found = True
                    break
        conffile.closed
        with open(conf, 'a') as conffile:
            conffile.write('%s\n' % NETSIM_STRING)
            if not found:
                conffile.write('\nServerName www.example.com:80\n')
            for ip in ip_list:
                conffile.write(APACHE_VIRTUAL_HOST_TEMPLATE % (ip, ip, doc_root, doc_root))
        conffile.closed
            

    except Exception as e:
        logging.getLogger(__name__).error(e)

def configure_apache_split_conf(ip_list, ports, ports_bak, sites_available, sites_enabled):
    try:
        # back up the existing ports.conf
        shutil.copyfile(ports, ports_bak)
        with open(ports, 'a') as portsfile:
            portsfile.write('%s\n' % NETSIM_STRING)
        portsfile.closed
            
        for ip in ip_list:
            # append virtual hosts to ports.conf
            with open(ports, 'a') as portsfile:
                    portsfile.write('\n\nNameVirtualHost %s:8080\n' % ip)
                    portsfile.write('Listen %s:8080' % ip)
            portsfile.closed

            # make a conf file for this virtual host
            confpath = os.path.join(sites_available, ip)
            with open(APACHE_UBUNTU_DEFAULT_SITE, 'r') as defaultfile:
                with open(confpath, 'w') as conffile:
                    for line in defaultfile:
                        if '<VirtualHost' in line:
                            conffile.write('<VirtualHost %s:8080>\n' % ip)
                        else:
                            conffile.write(line)
                conffile.closed
            defaultfile.closed

            # symlink conf file to sites-enabled
            linkpath = os.path.join(sites_enabled, ip)
            if not os.path.islink(linkpath):
                os.symlink(confpath, linkpath)

    except Exception as e:
        logging.getLogger(__name__).error(e)

# Prepare apache VirtualHost for each server ip in ip_list
def configure_apache(ip_list):
    if LINUX == 'Ubuntu':
        configure_apache_split_conf(ip_list, APACHE_UBUNTU_PORTS, APACHE_UBUNTU_PORTS_BAK,\
            APACHE_UBUNTU_SITES_AVAILABLE, APACHE_UBUNTU_SITES_ENABLED)
    elif LINUX == 'Fedora':
        configure_apache_single_conf(ip_list, APACHE_FEDORA_CONF,\
            APACHE_FEDORA_CONF_BAK, APACHE_FEDORA_DOC_ROOT)
    else:
        configure_apache_single_conf(ip_list, APACHE_RHEL_CONF,\
            APACHE_RHEL_CONF_BAK, APACHE_RHEL_DOC_ROOT)


def reset_apache_single_conf(ip_list, conf, conf_bak):
    try: 
        # restore ports.conf from backup
        if os.path.isfile(conf_bak):
            shutil.move(conf_bak, conf)
        else:
            logging.getLogger(__name__).warning('Could not find %s' % conf_bak)

        # TODO: clean this up
        found = False
        if os.path.isfile(conf):
            with open(conf, 'r') as conffile:
                for line in conffile:
                    if 'ServerName' in line and line[0] != '#':
                        found = True
                        break
            conffile.closed
        if not found:
            with open(conf, 'a') as conffile:
                conffile.write('\nServerName www.example.com:80\n')
            conffile.closed
    except Exception as e:
        logging.getLogger(__name__).error(e)

def reset_apache_split_conf(ip_list, ports, ports_bak, sites_available, sites_enabled):
    try:
        # restore ports.conf from backup
        if os.path.isfile(APACHE_UBUNTU_PORTS_BAK):
            shutil.move(APACHE_UBUNTU_PORTS_BAK, APACHE_UBUNTU_PORTS)
        else:
            logging.getLogger(__name__).warning('Could not find %s' % APACHE_UBUNTU_PORTS_BAK)

        # remove conf files
        for ip in ip_list:
            confpath = os.path.join(APACHE_UBUNTU_SITES_AVAILABLE, ip)
            if os.path.isfile(confpath):
                os.remove(confpath)

            linkpath = os.path.join(APACHE_UBUNTU_SITES_ENABLED, ip)
            if os.path.islink(linkpath):
                os.remove(linkpath)

    except Exception as e:
        logging.getLogger(__name__).error(e)


# Put apache back to normal
def reset_apache(ip_list):
    if LINUX == 'Ubuntu':
        reset_apache_split_conf(ip_list, APACHE_UBUNTU_PORTS, APACHE_UBUNTU_PORTS_BAK,\
            APACHE_UBUNTU_SITES_AVAILABLE, APACHE_UBUNTU_SITES_ENABLED)
    elif LINUX == 'Fedora':
        reset_apache_single_conf(ip_list, APACHE_FEDORA_CONF, APACHE_FEDORA_CONF_BAK)
    else:
        reset_apache_single_conf(ip_list, APACHE_RHEL_CONF, APACHE_RHEL_CONF_BAK)


def restart_apache_binary(bin):
    check_output('%s -k restart' % bin, shouldPrint=True)

def restart_apache_script(script):
    check_output('%s restart' % script, shouldPrint=False)

def restart_apache():
    if LINUX == 'Ubuntu':
        restart_apache_script(APACHE_UBUNTU)
    elif LINUX == 'Fedora':
        restart_apache_binary(APACHE_FEDORA)
    else:
        restart_apache_binary(APACHE_RHEL)
