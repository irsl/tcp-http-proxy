# Abstract

So this is a tiny helper tool that is able to wrap a TCP connection to a tunnel provided by a HTTP connect proxy,
all the thing fully transparently (iptables REDIRECT).

# Why?

My goal was to be able to intercept TLS sessions of my mobile phone using mitmproxy. 
The idea of running that awesome software on my openwrt router turned out to be too
naive, so I started looking for some proxy tool to do that. Solutions like tinyproxy or
redsocks didn't do the job (I actually wasted more time on evaluating the available tools
than writing this one), so I ended up implementing this thing. (Wanted to do that in
lua first using luaposix, but the version of the package on openwrt is way too outdated.)

# How to use this?

Setup a REDIRECT rule something like this:

```
iptables -t nat -D zone_lan_prerouting -p tcp -s 10.6.6.149 --dport 443 -j REDIRECT --to-port 8081
```

Execute this tool:
```
./tcp-http-proxy 8081 10.6.6.111 8080
```

Where 10.6.6.111:8080 is where mitmproxy is listening (eg. in a Kali VM).

# Remark

The binary was built for the ath79 MIPS SoC with musl libc.
