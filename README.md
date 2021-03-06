Trunks_versionSemaphores
========================

A solution to the multi-TRUNK's, one ETHER problem using semaphore.

The producer does the following repeatedly:
ETHER produce:
 * WAIT (boxesFull)
 * ACQUIRE (Mutext)
 * item ← PUTboxinQueue
 * RELEASE (Mutext)
 * SiGNAL (boxesAvailable)

The consumer does the following repeatedly:
TRUNK consume:
 * WAIT (boxesAvailable)
 * ACQUIRE (Mutext)
 * item ← GETboxFromQueue
 * RELEASE (Mutext)
 * SIGNAL (boxesFull)


  @brief: if CAPACITYETHER is superior to a multiple of CAPACITYTRUNK (CAPACITYETHER%CAPACITYTRUNK>0),
  subsequent boxes sent by ETHER are lost because TRUNKS won't have the capacity to process it
  e.g. CAPACITYETHER=  (CAPACITYTRUNK*4)+1
 
  On the other hand if CAPACITYETHER if less than the sum of CAPACITYTRUNK
  there will always be 1 or more TRUNKS waiting forever until self capacity
  e.g. CAPACITYETHER=  (CAPACITYTRUNK*4)-1 will let 1 TRUNK waiting
 
  BEST scenario is CAPACITYETHER multiple of CAPACITYTRUNK (CAPACITYETHER%CAPACITYTRUNK=0)
  This means that each and every box spawned by ETHER will be processed by 1 TRUNK
 
  ALSO , no matter if QUEUESIZE > CAPACITYETHER or QUEUESIZE < CAPACITYETHER.
