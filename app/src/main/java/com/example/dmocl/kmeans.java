package com.example.dmocl;


import java.util.concurrent.Semaphore;
import java.util.Random;
import java.lang.Float;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class kmeans {

  final static int maxcycles = 100000;

  private static boolean doabort = false;
  private static final Object LOCKA = new Object();
  private static final ReentrantReadWriteLock rrwl = new ReentrantReadWriteLock(true);


  public static void kmabort(){
    synchronized(LOCKA) {
      rrwl.writeLock().lock();
      doabort = true;
      rrwl.writeLock().unlock();
      kmabort_c();
    }
  }

  public static void kmresume(){
    synchronized(LOCKA) {
      rrwl.writeLock().lock();
      doabort = false;
      rrwl.writeLock().unlock();
      kmresume_c();
    }
  }



    static {
      System.loadLibrary("kmeans_c");
    }

    private static native void kmabort_c();
    private static native void kmresume_c();
    public static native short kmeans_c( short[] b, float[] data, float eps , int cluno, int features );
    public static native short kmeans_c_gpu( short[] b, float[] data, float eps , int cluno, int features, long[] e );
    public static native short kmeans_c_phtreads( short[] b, float[] data, float eps , int cluno,
                                                  int features, int cores, long[] e );



    private class kmeans_thread extends Thread {

      volatile private AtomicBoolean status = new AtomicBoolean(false);

      private int num = -1;
      private short[] b;
      private float[] data;
      private int cluno = -1;
      private int features = -1;
      private int start = 0;
      private int len = 0;
      private float[][] clucent = null;

      private Semaphore sem = new Semaphore( 0 );
      private Semaphore semret = new Semaphore( 0 );


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


      public void terminate(){
        status.set(true);
        this.WakeUp();
        //this.semret.release();
      }

      public boolean isTerminated(){
        return( status.get() );
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

            for( int i1=this.start; i1 < this.start+this.len; i1++ ) {

              float noxi = Float.POSITIVE_INFINITY;

              for( short i2=0; i2<cluno; i2++ ){

                float noxi2 = 0;

                for( int i3=0; i3<features; i3++ ){
                  noxi2 += Math.pow( clucent[i2][i3] - data[i1*features+i3], 2 );
                }

                if (noxi2<noxi){
                  noxi = noxi2;
                  b[i1] = i2;
                }
              }
            }

            semret.release();
          }
        }
      }

      public int getNum(){
        return( num );
      }

      public kmeans_thread( int num, short[] b, float[] data,
                            int cluno, int features, int start, int len, float[][] clucent ){

        this.num = num;
        this.b = b;
        this.data = data;
        this.cluno = cluno;
        this.features = features;
        this.start = start;
        this.len = len;
        this.clucent = clucent;

      }

    }




    public static short kmeans_st( short[] b, float[] data, float eps , int cluno, int features ) {

      short ret = 0;

      Random r = new Random();


      float[][] clucent = new float[cluno][features];
      float[][] newclucent = new float[cluno][features];
      int[] clusize = new int[cluno];


      for( int i1=0; i1<cluno; i1++ ){

        int cluxi = r.nextInt( b.length );

        for( int i2=0; i2<features; i2++ ){
          clucent[i1][i2] = data[cluxi*features+i2];
        }

      }

      int weiter = 0;
      int cycles = 0;

      while (weiter == 0){

        boolean temp = false;
        rrwl.readLock().lock();
        temp = doabort;
        rrwl.readLock().unlock();

        if (temp == true) {
          ret = -1;
          weiter = 1;
        }
        else {

          for (int i1 = 0; i1 < b.length; i1 += 1) {

            float noxi = Float.POSITIVE_INFINITY;

            for (short i2 = 0; i2 < cluno; i2++) {

              float noxi2 = 0;

              for (int i3 = 0; i3 < features; i3++) {
                noxi2 += Math.pow(clucent[i2][i3] - data[i1 * features + i3], 2);
              }

              if (noxi2 < noxi) {
                noxi = noxi2;
                b[i1] = i2;
              }
            }
          }


          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1][i2] = 0;
            }
            clusize[i1] = 0;
          }

          for (int i1 = 0; i1 < b.length; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1]][i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1][i2] /= clusize[i1];
            }
          }

          float newdist = 0;

          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += Math.pow(clucent[i1][i2] - newclucent[i1][i2], 2);
            }

            newdist += Math.sqrt(newdist2);
          }

          if ((newdist <= eps) || (cycles > maxcycles)) {
            weiter = 1;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              clucent[i1][i2] = newclucent[i1][i2];
            }
          }

          cycles++;

        }
      }

      return( ret );
    }






    public static short kmeans_threads( short[] b, float[] data, float eps , int cluno, int features,
                                       int cores, long[] ej ) throws InterruptedException {


      short ret = 0;

      Random r = new Random();


      float[][] clucent = new float[cluno][features];
      float[][] newclucent = new float[cluno][features];
      int[] clusize = new int[cluno];


      for( int i1=0; i1<cluno; i1++ ){

        int cluxi = r.nextInt( b.length );

        for( int i2=0; i2<features; i2++ ){
          clucent[i1][i2] = data[cluxi*features+i2];
        }
      }

      kmeans_thread kmth[] = new kmeans_thread[cores];

      int stepper = (b.length / cores) + 1;
      int starter = 0;
      int reminder = b.length;

      for( int i1=0; i1<cores; i1++ ){

        if (reminder < stepper){
          stepper = reminder;
        }

        kmth[i1] = new kmeans().new kmeans_thread( i1, b, data, cluno, features, starter, stepper,
            clucent );
        kmth[i1].start();

        starter += stepper;
        reminder -= stepper;

      }

      long t1 = System.nanoTime();

      int weiter = 0;
      int cycles = 0;

      while (weiter == 0){

        boolean temp = false;
        //rrwl.readLock().lock();
        //temp = doabort;
        //rrwl.readLock().unlock();

        if (temp == true) {
          ret = -1;
          weiter = 1;
        }
        else {

          for (int i1 = 0; i1 < cores; i1++) {
            kmth[i1].WakeUp();
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            clusize[i1] = 0;
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1][i2] = 0;
            }
          }

          for (int i1 = 0; i1 < cores; i1++) {
            if (kmth[i1].WaitFinished() < 0) {
              ret = -1;
              weiter = 1;
              break;
            }
          }

          if (ret < 0) {
            break;
          }

          for (int i1 = 0; i1 < b.length; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[b[i1]][i2] += data[i1 * features + i2];
            }
            clusize[b[i1]]++;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              newclucent[i1][i2] /= clusize[i1];
            }
          }

          float newdist = 0;

          for (int i1 = 0; i1 < cluno; i1++) {

            float newdist2 = 0;

            for (int i2 = 0; i2 < features; i2++) {
              newdist2 += Math.pow(clucent[i1][i2] - newclucent[i1][i2], 2);
            }

            newdist += Math.sqrt(newdist2);
          }

          if ((newdist <= eps)  || (cycles > maxcycles)) {
            weiter = 1;
          }

          for (int i1 = 0; i1 < cluno; i1++) {
            for (int i2 = 0; i2 < features; i2++) {
              clucent[i1][i2] = newclucent[i1][i2];
            }
          }

          cycles++;
        }
      }

      ej[0] = System.nanoTime() - t1;

      for( int i1=0; i1<cores; i1++){

        kmth[i1].interrupt();
        kmth[i1].terminate();

        while (kmth[i1].isAlive()){
          kmth[i1].interrupt();
          try {
            kmth[i1].join();
          }
          catch( InterruptedException e ){
          }
        }
      }

      return( ret );
    }

}


