# smoke_gateway
I recently saw an advertisement for a Thermoworks Smoke Gateway. A device that allows you to connect the Smoke to your WiFi network. Since the Smoke transmits wirelessly, 
the gateway is just a bridging device between that signal, and the wifi connection. At $100, I was inspiried to create my own open source version. 

Thanks to the helpful tips from skytoastar on this (project post)[https://hackaday.io/project/160386-blanket-the-smoke-signal-gateway]. Sleuthing around the FCC website they were
able to determine the wireless chip used was an nrf24l01+. As well as some other important parameters required to receive data from the Smoke. After reading this I realized 
I had a handful of nrf24s, and an ESP32 just waiting to turn into a gateway. The one thing I did not have was the address the smoke was transmitting with. skytoastar used an 
expensive SDR to sniff the address. I didn't have one of those, but the project post suggested that an nrf24 could be configured to sniff out the address of transmitting devices. 

I found this (blog post)[https://travisgoodspeed.blogspot.com/2011/02/promiscuity-is-nrf24l01s-duty.html] describing how to setup an nrf24 into promiscuous mode. Basically,
the nrf24 will normally attempt to match messages that start with a preamble (alternating 1/0 as least one byte long. ex. 0x55/0xaa), then a 3-5 byte address, and end with a a-2 byte CRC. The address width can be configured
with the SETUP_AW register in the nrf24. However, the datasheet indicates that wirting a value of '0' to this register is illegal - but the researcher discovered that this 
actually enables a special mode that can be used as a semi-promiscuous mode. 
