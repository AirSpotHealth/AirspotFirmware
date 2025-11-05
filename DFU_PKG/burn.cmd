nrfjprog -f NRF52 --eraseall
nrfjprog -f NRF52 --program "final.hex" --verify
nrfjprog -f NRF52 --reset

pause
