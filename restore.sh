#Whoops, Autoremove nuked the system.
#This file will build a .sh script to restore
#!/bin/bash

echo '#!/bin/bash' > restore
echo sudo apt-get install `grep Remove /var/log/apt/history.log | tail -1 | sed -e 's|Remove: ||g' -e 's|([^)]*)||g' -e 's|:[^ ]* ||g' -e 's|,||g'` >> restore
chmod +x restore
