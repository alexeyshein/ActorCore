#include "catch.hpp"

#include <DataPublisherFunctor.hpp>
#include <string>

   struct DataShare
   {
         rf::DataPublisherFunctor<std::string> publisher1;
         rf::DataPublisherFunctor<std::string> publisher2;
   };

   template<class T>
   struct Receiver
   {
         void receiver1(T str) { str1 = str; };
         void receiver2(T str) { str2 = str; };
         T str1;
         T str2;
   };


TEST_CASE( "DataPublisherFunctor", "[std::string]" ) {

   
   DataShare  share;
   Receiver<std::string> receiver;
     

   SECTION( "Publish null SyncMode" ) {
       share.publisher1.SetAsyncMode(false);
       std::string str = "Str1";
       share.publisher1.Notify(str);
       REQUIRE( str != receiver.str1 );
       REQUIRE( str != receiver.str2 );
   }

      SECTION( "Publish null ASyncMode" ) {
        share.publisher1.SetAsyncMode(true);
       std::string str = "Str1";
       share.publisher1.Notify(str);
       REQUIRE( str != receiver.str1 );
       REQUIRE( str != receiver.str2 );
   }

   SECTION( "Publisher1 to receiver1 SyncMode" ) {
       share.publisher1.SetAsyncMode(false);
       std::string str = "Str1";
       using std::placeholders::_1;
       share.publisher1.Attach(1, std::bind( &Receiver<std::string>::receiver1, &receiver, _1 ));
       share.publisher1.Notify(str);
       REQUIRE( str == receiver.str1 );
       REQUIRE( str != receiver.str2 );
   }

      SECTION( "Publisher1 to receiver1 ASyncMode" ) {
        share.publisher1.SetAsyncMode(true);
       std::string str = "Str1";
       using std::placeholders::_1;
       share.publisher1.Attach(1, std::bind( &Receiver<std::string>::receiver1, &receiver, _1 ));
       share.publisher1.Notify(str);
       std::this_thread::sleep_for(std::chrono::milliseconds(1));
       REQUIRE( str == receiver.str1 );
       REQUIRE( str != receiver.str2 );
   }

      SECTION( "publisher1 to receiver1 and publisher2 to receiver2" ) {
       share.publisher1.SetAsyncMode(false);
       share.publisher2.SetAsyncMode(false);

       std::string str1 = "Str1+";
       std::string str2 = "Str2+";
       using std::placeholders::_1;
       share.publisher1.Attach(1, std::bind( &Receiver<std::string>::receiver1, &receiver, _1 ));
       share.publisher1.Notify(str1);
       REQUIRE(share.publisher1.NumObservers() == 1);
       share.publisher2.Attach(2, std::bind( &Receiver<std::string>::receiver2, &receiver, _1 ));
       share.publisher2.Notify(str2);
       REQUIRE(share.publisher2.NumObservers() == 1);
       REQUIRE( str1 == receiver.str1 );
       REQUIRE( str2 == receiver.str2 );
   }



    SECTION( "(Attach publisher 1 to str 1 and to str 2) AND (Detach str2)" ) { // Проверяем что отсоединяется только требуемый
        share.publisher1.SetAsyncMode(false);
       std::string str1 = "Str1+";
       std::function<void(const std::string&)>  functor1 = std::bind( &Receiver<std::string>::receiver1, &receiver, std::placeholders::_1 );
       std::function<void(const std::string&)>  functor2 =  std::bind( &Receiver<std::string>::receiver2, &receiver, std::placeholders::_1 );  
       auto &targetType1 = functor1.target_type();
       auto &targetType2 = functor2.target_type();

       share.publisher1.Attach(1, functor1);
       share.publisher1.Attach(12, functor2);
       REQUIRE(share.publisher1.NumObservers() == 2);
       share.publisher1.Notify(str1);
       REQUIRE( str1 == receiver.str1 );
       REQUIRE( str1 == receiver.str2 );
      
       share.publisher1.Detach(12);
       REQUIRE(share.publisher1.NumObservers() == 1);
       str1 = "Str1++";
       share.publisher1.Notify(str1);
       REQUIRE( str1 == receiver.str1 );
       REQUIRE( str1 != receiver.str2 );
   }

}






