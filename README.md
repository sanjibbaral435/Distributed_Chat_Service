# Distributed_Chat_Service
This is the command line based fault tolerant distributed chat service


System Requirement:
Linux should have x-terminal package installed (We are running x-terminal-emulator command to start new process)


Process to Run the Solution:
1. Copy all the source to all 3 serves (lenss-comp1, lenss-comp3, lenss-comp4)
2. We are considering lenss-comp4 as master machine. So once all the files copies, run ./lenss4_master.sh script to run start the master process and worker process of the MASTER machine. (Wait for 20 seconds to start all the terminals)

3. Run lenss1.sh and lenss3.sh in the respective machines (we have hard coded the IP). We are considering lenss1 and lenss3 as slave machines which will have few worker processes running.

4. Once all machines and setups are up, run client codes from any machine (./fbc -u <username>)

5. You can create as many clients as possible and run the test cases.




For any quirks or steps please reach out to us
sanjibbaral.nit@gmail.com