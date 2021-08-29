package com.example.dmocl;

import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class dbscan {

    private static boolean doabort = false;
    private static final Object LOCK = new Object();
    private static final ReentrantReadWriteLock rrwl = new ReentrantReadWriteLock(true);

    public static void dbscanabort(){
      synchronized(LOCK) {
        rrwl.writeLock().lock();
        doabort = true;
        rrwl.writeLock().unlock();
        dbscanabort_c();
      }
    }

    public static void dbscanresume(){
      synchronized(LOCK) {
        rrwl.writeLock().lock();
        doabort = false;
        rrwl.writeLock().unlock();
        dbscanresume_c();
      }
    }

    static {
      System.loadLibrary("dbscan_c");
    }

    private static native void dbscanabort_c();
    private static native void dbscanresume_c();
    public static native short dbscan_c( short[] b, float[] data, float eps , int kk, int features );
    public static native short dbscan_c_gpu( short[] b, float[] data, float eps , int kk, int features, long[] e );
    public static native short dbscan_c_phtreads( short[] b, float[] data, float eps ,
                             int kk, int features, int cores, long[] e );


    private class dbscan_thread1 extends Thread {

      private volatile AtomicBoolean status = new AtomicBoolean(false);

      private int num = -1;
      private short[] b;
      private float[] data;
      private float epseps = -1;
      private int kk = -1;
      private int features = -1;
      private int start = 0;
      private int len = 0;

      private Semaphore sem = new Semaphore( 0 );
      private Semaphore semret = new Semaphore( 0 );

      private final Object LOCK2 = new Object();  //access itemcounter & cmpto
      private volatile int itemcounter = 0;
      private volatile int cmpto = -1;


      public int WaitFinished() {

        boolean hasfinished = false;

        while ((!hasfinished) && (!status.get())){

          try {
            this.semret.acquire();
            hasfinished = true;
          } catch (InterruptedException ignored) {
          }
        }

        return( 0 );
      }


      public void WakeUp(){
        this.sem.release();
      }


      public void setCmpTo( int cmpto ){
        synchronized (LOCK2) {
          this.cmpto = cmpto;
        }
      }

      public void terminate(){
        status.set( true );
        this.WakeUp();
        //this.semret.release();
      }

      public boolean isTerminated(){
        return( status.get() );
      }

      public int getItemCounter() {

        int ret = 0;
        synchronized (LOCK2){
          ret = this.itemcounter;
        }
        return( ret );
      }

      public void run(){


        while (!status.get()){

          boolean hasbeeninterrupted = false;

          try {
            this.sem.acquire();
          }
          catch( InterruptedException e ){
            hasbeeninterrupted = true;
          }

          if ((!status.get()) && (!hasbeeninterrupted)) {

            synchronized (LOCK2) {
              int itemcounter = 0;

              for (int i2 = this.start; i2 < this.start + this.len; i2++) {

                b[i2] &= 65535 - 6;     // Bit 2+3 löschen

                float s = 0;

                for (int i3 = 0; i3 < this.features; i3++) {
                  s += Math.pow(this.data[i2 * this.features + i3] - this.data[this.cmpto * this.features + i3], 2);
                }

                if (s <= this.epseps) {

                  b[i2] |= 2;   // Distanzbit (3) setzen
                  itemcounter++;

                }

              }

              this.itemcounter = itemcounter;
            }

            this.semret.release();
          }
        }
      }

      public int getNum(){
        return( num );
      }

      public dbscan_thread1( int num, short[] b, float[] data, float epseps,
                             int kk, int features, int start, int len ){

        this.num = num;
        this.b = b;
        this.data = data;
        this.epseps = epseps;
        this.kk = kk;
        this.features = features;
        this.start = start;
        this.len = len;

      }

    }




    private class dbscan_thread2 extends Thread {

      private volatile AtomicBoolean status = new AtomicBoolean(false);


      private int num = -1;
      private short[] b;
      private float[] data;
      private float epseps = -1;
      private int kk = -1;
      private int features = -1;
      private int start = 0;
      private int len = 0;

      private Semaphore sem = new Semaphore( 0 );
      private Semaphore semret = new Semaphore( 0 );

      private final Object LOCK2 = new Object();  //access itemcounter & cmpto
      private volatile int cmpto = -1;
      private volatile int itemcounter = 0;


      public void WakeUp(){
        this.sem.release();
      }


      public int WaitFinished() {

        boolean hasfinished = false;

        while ((!hasfinished) && (!status.get())){

          try {
            this.semret.acquire();
            hasfinished = true;
          } catch (InterruptedException ignored) {
          }
        }

        return( 0 );
      }


      public void setCmpTo( int cmpto ){
        synchronized (LOCK2) {
          this.cmpto = cmpto;
        }
      }

      public void terminate(){
        status.set(true);
        this.WakeUp();
        //this.semret.release();
      }

      public boolean isTerminated(){
        return( status.get() );
      }

      public int getItemCounter() {
        int ret = 0;

        synchronized (LOCK2){
          ret = this.itemcounter;
        }
        return( ret );

      }

      public void run(){

        while (!status.get()){

          boolean hasbeeninterrupted = false;

          try {
            this.sem.acquire();
          }
          catch( InterruptedException e ){
            hasbeeninterrupted = true;
          }

          if ((!status.get()) && (!hasbeeninterrupted)) {

            synchronized (LOCK2) {
              int itemcounter = 0;

              for (int i2 = this.start; i2 < this.start + this.len; i2++) {

                b[i2] &= 65535 - 4;     // Bit 3 löschen

                float s = 0;

                for (int i3 = 0; i3 < this.features; i3++) {
                  s += Math.pow(this.data[i2 * this.features + i3] - this.data[this.cmpto * this.features + i3], 2);
                }

                if (s <= this.epseps) {

                  b[i2] |= 4;   // Distanzbit (3) setzen
                  itemcounter++;

                }

              }

              this.itemcounter = itemcounter;
            }

            this.semret.release();
          }
        }
      }

      public int getNum(){
        return( num );
      }

      public dbscan_thread2( int num, short[] b, float[] data, float epseps,
                             int kk, int features, int start, int len ){

        this.num = num;
        this.b = b;
        this.data = data;
        this.epseps = epseps;
        this.kk = kk;
        this.features = features;
        this.start = start;
        this.len = len;

      }

    }







    private static short expandCluster_st( int key, short clusternumber, short[] b,
                                          float[] data, float epseps, int kk, int features ) {

      short ret = 0;

      b[key] &= 7;            // Bits 3-15 löschen

      b[key] |= (clusternumber << 3);
      // Clusternummer abspeichern

      byte weiter = 0;

      while (weiter==0) {

        weiter = 1;

        boolean temp = false;
        rrwl.readLock().lock();
        temp = doabort;
        rrwl.readLock().unlock();

        if (temp == true) {
          ret = -1;
          weiter = 2;
        }


        if (weiter==1) {

          for (int i1 = 0; i1 < b.length; i1++) {

             if ((b[i1] & 3) == 2) {

              b[i1] |= 1;

              int itemcounter2 = 0;

              for (int i2 = 0; i2 < b.length; i2++) {

                b[i2] &= ~((short) 4);      // Bit 3 löschen

                float s = 0;

                for (int i3 = 0; i3 < features; i3++) {
                  s += Math.pow(data[i2 * features + i3] - data[i1 * features + i3], 2);
                }

                if (s <= epseps) {

                  b[i2] |= 4;                       // Distanzbit (3) setzen
                  itemcounter2++;

                }
              }

              if (itemcounter2 >= kk) {

                for (int i2 = 0; i2 < b.length; i2++) {

                  if ((b[i2] & 4) != 0) {

                    if ((b[i2] & 2) == 0) {

                      b[i2] |= 2;         // Bit 2 setzen
                      weiter = 0;
                    }
                  }
                }
              }

              for (int i2 = 0; i2 < b.length; i2++) {

                b[i2] &= ~((short) 4);      // Bit 3 löschen

              }

              if ((b[i1] >> 3) == 0) {

                b[i1] |= (clusternumber << 3);

              }
            }
          }

          if (ret<0){
            break;
          }
        }
      }

      return( ret );
    }



    public static short dbscan_st( short[] b, float[] data, float eps , int kk, int features ) {

      short clusternumber = 0;
      final float epseps = eps*eps;

      for( int i1=0; i1<b.length; i1++ ) {
        b[i1] = 0;
      }

      for( int i1=0; i1<b.length; i1 += 1 ){

        if ((b[i1]&1) == 0){            // visited ?

          boolean temp = false;
          rrwl.readLock().lock();
          temp = doabort;
          rrwl.readLock().unlock();

          if (temp == true){
            clusternumber = -1;
            break;
          }

          b[i1] |= 1;       // visited (1) setzen

          int itemcounter = 0;


          for( int i2 = 0; i2 < b.length; i2++ ) {

            b[i2] &= ~((short) 6);     // Bits 2 und 3 löschen

            float s = 0;

            for( int i3 = 0; i3 < features; i3++ ){
              s += Math.pow( data[i2*features+i3] - data[i1*features+i3], 2 );
            }

            if (s <= epseps) {

              b[i2] |= 2;        // Distanzbit (2) setzen
              itemcounter++;

            }
          }

          if (itemcounter < kk) {
            b[i1] &= 7;  // Cluster Nr. 0 = Noise
          }
          else
          {
            if (clusternumber == 4095){
              clusternumber = -256;
              break;
            }

            clusternumber += 1;

            short ret2 = expandCluster_st( i1, clusternumber, b, data, epseps, kk, features );

            if (ret2<0){
              clusternumber = ret2;
              break;
            }
          }

        }
      }

      if (clusternumber>=0) {
        for (int i1 = 0; i1 < b.length; i1++) {
          b[i1] = (short) (b[i1] >> 3);      // just retain cluster number, delete status bits
        }
      }

      return( clusternumber );
    }





    private static short expandCluster_threads( int key, short clusternumber, short[] b,
                                               float[] data, float epseps, int kk, int features, int cores, dbscan_thread2[] dbth2 ) throws InterruptedException  {

      short ret = 0;

      b[key] &= 7;            // Bits 3-15 löschen

      b[key] |= (clusternumber << 3);
      // Clusternummer abspeichern

      byte weiter = 0;

      while (weiter==0) {

        weiter = 1;

        boolean temp = false;
        rrwl.readLock().lock();
        temp = doabort;
        rrwl.readLock().unlock();

        if (temp == true){
          ret = -1;
          weiter = 2;
        }

        if (weiter==1) {


          for (int i1 = 0; i1 < b.length; i1++) {

            if ((b[i1] & 3) == 2) {

              b[i1] |= 1;

              for (int i2 = 0; i2 < cores; i2++) {
                dbth2[i2].setCmpTo(i1);
                dbth2[i2].WakeUp();
              }

              int itemcounter2 = 0;

              for (int i2 = 0; i2 < cores; i2++) {
                if (dbth2[i2].WaitFinished()<0){
                  ret = -1;
                  break;
                }
                itemcounter2 += dbth2[i2].getItemCounter();
              }

              if (ret<0){
                break;
              }


              if (itemcounter2 >= kk) {

                for (int i2 = 0; i2 < b.length; i2++) {

                  if ((b[i2] & 4) != 0) {

                    if ((b[i2] & 2) == 0) {

                      b[i2] |= 2;         // Bit 2 setzen
                      weiter = 0;
                    }
                  }
                }
              }

              for (int i2 = 0; i2 < b.length; i2++) {

                b[i2] &= ~((short) 4);      // Bit 3 löschen

              }

              if ((b[i1] >> 3) == 0) {

                b[i1] |= (clusternumber << 3);

              }
            }
          }

          if (ret<0){
            break;
          }
        }
      }

      return( ret );
    }


    public static short dbscan_threads( short[] b, float[] data, float eps , int kk, int features,
                                        int cores, long[] ej ) throws InterruptedException {


      short clusternumber = 0;
      final float epseps = eps*eps;

      dbscan_thread1 dbth1[] = new dbscan_thread1[cores];
      dbscan_thread2 dbth2[] = new dbscan_thread2[cores];

      int stepper = (b.length / cores) + 1;
      int starter = 0;
      int reminder = b.length;

      for( int i1=0; i1<cores; i1++){

        if (reminder < stepper){
          stepper = reminder;
        }

        dbth1[i1] = new dbscan().new dbscan_thread1( i1, b, data, epseps, kk, features, starter, stepper );
        dbth1[i1].start();
        dbth2[i1] = new dbscan().new dbscan_thread2( i1, b, data, epseps, kk, features, starter, stepper );
        dbth2[i1].start();

        starter += stepper;
        reminder -= stepper;

      }

      long t1 = System.nanoTime();

      for( int i1=0; i1<b.length; i1++ ) {
        b[i1] = 0;
      }


      for( int i1=0; i1<b.length; i1 += 1 ){

        if ((b[i1]&1) == 0){            // visited ?

          boolean temp = false;
          rrwl.readLock().lock();
          temp = doabort;
          rrwl.readLock().unlock();

          if (temp == true){
            clusternumber = -1;
            break;
          }


          b[i1] |= 1;       // visited (1) setzen

          for( int i2=0; i2<cores; i2++ ){
            dbth1[i2].setCmpTo( i1 );
            dbth1[i2].WakeUp();
          }

          int itemcounter = 0;

          for( int i2=0; i2<cores; i2++ ){
            if (dbth1[i2].WaitFinished()<0){
              clusternumber = -1;
              break;
            }
            itemcounter += dbth1[i2].getItemCounter();
          }

          if (clusternumber<0){
            break;
          }

          if (itemcounter < kk) {
            b[i1] &= 7;  // Cluster Nr. 0 = Noise
          }
          else
          {
            if (clusternumber == 4095){
              clusternumber = -1;
              break;
            }

            clusternumber += 1;

            short ret2 = expandCluster_threads( i1, clusternumber, b, data, epseps, kk, features, cores, dbth2 );

            if (ret2<0){
              clusternumber = ret2;
              break;
            }
          }

        }
      }

      if (clusternumber>=0) {
        for (int i1 = 0; i1 < b.length; i1++) {
          b[i1] = (short) (b[i1] >> ((short) 3));      // just retain cluster number, delete status bits
        }
      }

      ej[0] = System.nanoTime() - t1;

      for( int i1=0; i1<cores; i1++){

        while (dbth1[i1].isAlive()){

          dbth1[i1].terminate();
          dbth1[i1].interrupt();

          try {
            dbth1[i1].join(1000);
          }
          catch( InterruptedException ignored){
          }
        }

        while (dbth2[i1].isAlive()){

          dbth2[i1].terminate();
          dbth2[i1].interrupt();

          try {
            dbth2[i1].join(1000);
          }
          catch( InterruptedException ignored){
          }
        }

      }


      return( clusternumber );
    }

}
