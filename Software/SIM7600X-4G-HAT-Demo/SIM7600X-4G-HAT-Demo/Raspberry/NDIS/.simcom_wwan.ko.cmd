cmd_/home/pi/SIM7X00_NDIS/wwan/simcom_wwan.ko := ld -r  -EL -T ./scripts/module-common.lds -T ./arch/arm/kernel/module.lds  --build-id  -o /home/pi/SIM7X00_NDIS/wwan/simcom_wwan.ko /home/pi/SIM7X00_NDIS/wwan/simcom_wwan.o /home/pi/SIM7X00_NDIS/wwan/simcom_wwan.mod.o ;  true