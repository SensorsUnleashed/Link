###### Working setup (BBB and Server) ######
On remote computer add route to the border router
Add a mask so, that all sensors can be pinged
ip route add fd00::/64 via fd81:3daa:fb4a:f7ae:d239:72ff:fe4c:b375
sudo ip route add fd00::/64 via fd81:3daa:fb4a:f7ae:d239:72ff:fe4c:b375
ping6 fd00::212:4b00:05af:82b7


On BBB
sudo ./tunslip6 -L -v2 -s /dev/ttyACM0 fd00::1/64

wget -6 "http://[fd00::212:4b00:60d:9aa4]/
cat index.html

Setup the routing 
sysctl -w net.ipv6.conf.all.forwarding=1
ip route add fd00::/64 dev tun0 metric 1

if the global address is gone
ip addr add fd81:3daa:fb4a:f7ae:d239:72ff:fe4c:b375/64 dev eth0


When working, this is the routing tables
Server:
root@server:/home/araldit# route -A inet6
Kernel IPv6 routing table
Destination                    Next Hop                   Flag Met Ref Use If
fd00::/64                      fd81:3daa:fb4a:f7ae:d239:72ff:fe4c:b375 UG   1024 1    10 enp0s25
fd81:3daa:fb4a:f7ae::/64       ::                         UAe  256 0     2 enp0s25
fe80::/64                      ::                         U    256 0     0 enp0s25
::/0                           fe80::6a1:51ff:fec7:70ed   UGDAe 1024 2  1560 enp0s25
::/0                           ::                         !n   -1  1  1576 lo
::1/128                        ::                         Un   0   3568201 lo
fd81:3daa:fb4a:f7ae:224:1dff:feca:8938/128 ::                         Un   0   2    13 lo
fe80::224:1dff:feca:8938/128   ::                         Un   0   2    12 lo
ff00::/8                       ::                         U    256 2138274 enp0s25
::/0                           ::                         !n   -1  1  1576 lo


BBB routing table:
root@beaglebone:/home/debian# route -A inet6
Kernel IPv6 routing table
Destination                    Next Hop                   Flag Met Ref Use If
fd00::/64                      [::]                       U    1   1   480 tun0
fd00::/64                      [::]                       U    256 0     0 eth0
fd81:3daa:fb4a:f7ae::/64       [::]                       UAe  256 1    36 eth0
fe80::/64                      [::]                       U    256 0     0 eth0
fe80::/64                      [::]                       U    256 0     0 tun0
[::]/0                         [::]                       !n   -1  1   571 lo
localhost/128                  [::]                       Un   0   2     5 lo
fd00::/128                     [::]                       Un   0   1     0 lo
fd00::1/128                    [::]                       Un   0   2    73 lo
fd81:3daa:fb4a:f7ae::/128      [::]                       Un   0   1     0 lo
fd81:3daa:fb4a:f7ae:d239:72ff:fe4c:b375/128 [::]                       Un   0   2    37 lo
fe80::/128                     [::]                       Un   0   1     0 lo
fe80::/128                     [::]                       Un   0   1     0 lo
fe80::1/128                    [::]                       Un   0   1     0 lo
fe80::4a2c:b0ce:7752:87f2/128  [::]                       Un   0   1     0 lo
fe80::d239:72ff:fe4c:b375/128  [::]                       Un   0   2    66 lo
ff00::/8                       [::]                       U    256 1   183 eth0
ff00::/8                       [::]                       U    256 0     0 tun0
[::]/0                         [::]                       !n   -1  1   571 lo


