### Setup debian linux for dummies

1) download your prefered version of debian linux
2) install linux
3) update upgrade apt-get and apt
    ```
    sudo apt-get update
    sudo apt-get upgrade
    sudo apt update
    sudo apt upgrade
    ```
4) install pip and setuptool for python2 and python3
    ```
    sudo apt-get install python-pip
    sudo apt-get install python3-pip
    sudo apt-get install python-setuptools
    sudo apt-get install python3-setuptools
    ```
5) create "alias python=python3" inside of bashrc or profilerc
6) add the user to dialout group
    ```
    sudo usermod -a -G dialout $USER
    ```
7) reebot your pc
8) again update upgrade apt-get and apt


### enjoy your debain distro 
