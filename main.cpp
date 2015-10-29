#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <QTime>

using namespace std;

struct Record{
    int number;
    char bs;
    char lm;
    int volume;
    int price;
};

bool sellPred(const Record &f, const Record s)
{
    //Сортировка для списка продаж
    //сначала рыночные, затем лимитные заявки
    //затем по цене от !!!большей к меньшей!!!(рыночные не сортируются)
    //затем по порядковому номеру

    if(f.lm > s.lm) return true;
    if(f.lm < s.lm) return false;
    if((f.lm == 'L' && s.lm =='L') && f.price < s.price) return false;
    if((f.lm == 'L' && s.lm =='L') && f.price > s.price) return true;
    return f.number < s.number;
}

bool buyPred(const Record &f, const Record s)
{
    //Сортировка для списка покупки
    //сначала рыночные, затем лимитные заявки
    //затем по цене от !!!меньшей к большей!!!(рыночные не сортируются)
    //затем по порядковому номеру

    if(f.lm > s.lm) return true;
    if(f.lm < s.lm) return false;
    if((f.lm == 'L' && s.lm =='L') && f.price > s.price) return false;
    if((f.lm == 'L' && s.lm =='L') && f.price < s.price) return true;
    return f.number < s.number;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cout << "Start time: " << QTime::currentTime().second() << "  " << QTime::currentTime().msec() << endl;

    fstream originalFile("original111.csv" , ios::in);
    fstream outputFile("output.csv", ios::out);

    if(!originalFile)
    {
        cout << "File is not exist... Exit from programm";
        outputFile << "FAILED";
        originalFile.close();
        outputFile.close();
        return 1;
    }

    int minSellPrice = 100;
    int maxBuyPrice = 0;

    vector<Record> BuyList;
    vector<Record> SellList;

    BuyList.reserve(1000000);
    SellList.reserve(1000000);

    string tempStr = "";
    Record rec ;


    getline(originalFile, tempStr);

    //Считывание файла построчно, заполнение списков для продажи и покупки
    cout << "Start reading file original.csv" << endl;
    while(originalFile)
    {
        tempStr.clear();
        getline(originalFile, tempStr);
        stringstream ss;
        ss.str(tempStr);

        try{
            getline(ss, tempStr, ';');
            rec.number = stoi(tempStr);

            getline(ss, tempStr, ';');
            if(tempStr.size() == 1) rec.bs = tempStr.at(0);
            else continue;

            getline(ss, tempStr, ';');
            if(tempStr.size() == 1) rec.lm = tempStr.at(0);
            else continue;

            getline(ss, tempStr, ';');
            rec.volume = stoi(tempStr);

            getline(ss, tempStr);
            rec.price = stoi(tempStr);
        }
        catch(invalid_argument)
        {
            //cout << "invalid_argument..." << endl;
            continue;
        }

        if((rec.bs != 'B' && rec.bs != 'S') ||
                (rec.lm != 'L' && rec.lm != 'M') || rec.volume < 1) continue;
        if(rec.lm == 'L' && rec.price <= 0) continue;

        if(rec.bs == 'B')
        {
            BuyList.push_back(rec);
            if(rec.lm == 'L' && rec.price > maxBuyPrice) maxBuyPrice = rec.price;
        }

        if(rec.bs == 'S')
        {
            SellList.push_back(rec);
            if(rec.lm == 'L' && rec.price < minSellPrice) minSellPrice = rec.price;
        }
    }

    cout << "Stop reading file original.csv" << endl << endl;

    //cout << "Stop reading file time: " << QTime::currentTime().second() << "  " << QTime::currentTime().msec() << endl;

    //Если нет пересечения по ценам или все заявки рыночные - завершаем программу
    if(maxBuyPrice < minSellPrice || (minSellPrice == 0 && maxBuyPrice == 0))
    {
        outputFile << "FAILED";
        cout << "There is no crossing prices..." << endl;
        cout << "Auction failed! Finishing programm";
        originalFile.close();
        outputFile.close();
        return 1;
    }

    //Сортировка списков в соответствии с критериями
    sort(SellList.begin(), SellList.end(), sellPred);
    sort(BuyList.begin(), BuyList.end(), buyPred);


    //Определение цены, при которой объем аукциона в деньгах будет максимальным

    //Подсчет объема рыночных сделок, т.к. они всегда учавствуют в аукционе

    int sellMarketVolume = 0;
    auto psell = SellList.begin();
    while(psell->lm != 'L') sellMarketVolume += (*psell++).volume;

    int buyMarketVolume = 0;
    auto pbuy = BuyList.begin();
    while(pbuy->lm != 'L') buyMarketVolume += (*pbuy++).volume;

    int mostEfficientPrice = 0;
    long long int maxVolumeInMoney = 0;

    for(int i = minSellPrice; i <= maxBuyPrice; i++)
    {
        vector<Record>::iterator psellmin = find_if(psell, SellList.end(), [&i](const Record &rec){return rec.price <= i;});
        vector<Record>::iterator pbuymax = find_if(pbuy, BuyList.end(), [&i](const Record &rec){return rec.price >= i;});

         long long int sellVolume = 0;
        for(auto plimsell = psellmin; plimsell != SellList.end(); plimsell++) sellVolume += (*plimsell).volume;
        sellVolume += sellMarketVolume;

        long long int buyVolume = 0;
        for(auto plimbuy = pbuymax; plimbuy != BuyList.end(); plimbuy++) buyVolume += (*plimbuy).volume;
        buyVolume += buyMarketVolume;

        long long int volumeInMoney = min(sellVolume, buyVolume) * i;
        if(volumeInMoney > maxVolumeInMoney)
        {
            mostEfficientPrice = i;
            maxVolumeInMoney = volumeInMoney;
        }
    }

    if(!maxVolumeInMoney)
    {
        outputFile << "FAILED";
        cout << "Max price == 0" << endl;
        cout << "Auction failed! Finishing programm";
        originalFile.close();
        outputFile.close();
        //cout << "Volume in money = 0 " << endl;
        return 1;
    }

    cout << "Most Efficient Price = " << mostEfficientPrice << endl << endl;

    psell = SellList.begin();
    pbuy = BuyList.begin();

    outputFile << "OK" << ';'
               << mostEfficientPrice << ';'
               << maxVolumeInMoney  << ';'
               << endl;

    //совершение аукциона по наиболее оптимальной цене
    cout << "Start holding auction" << endl;

    while(psell != SellList.end() && pbuy != BuyList.end())
    {
        while(psell->lm == 'L' && psell->price > mostEfficientPrice && psell != SellList.end()) psell++;
        while(pbuy->lm == 'L' && pbuy->price < mostEfficientPrice && pbuy != BuyList.end()) pbuy++;

        if(psell->volume > pbuy->volume)
        {
            outputFile << pbuy->number << ';'
                       << psell->number << ';'
                       << pbuy->volume << ';'
                       << (pbuy->volume)*mostEfficientPrice
                       << endl;
            psell->volume = psell->volume - pbuy->volume;
            pbuy++;
        }

        if(pbuy->volume > psell->volume)
        {
            outputFile << pbuy->number << ';'
                       << psell->number << ';'
                       << psell->volume << ';'
                       << (psell->volume)*mostEfficientPrice
                       << endl;
            pbuy->volume = pbuy->volume - psell->volume;
            psell++;
        }

        if(pbuy->volume == psell->volume)
        {
            outputFile << pbuy->number << ';'
                       << psell->number << ';'
                       << psell->volume << ';'
                       << (psell->volume)*mostEfficientPrice
                       << endl;
            psell++;
            pbuy++;
        }
    }

    originalFile.close();
    outputFile.close();

    cout << "Auction completed successfully!!!" << endl;
    cout << "Finish time: " << QTime::currentTime().second() << "  " << QTime::currentTime().msec() << endl;

    return a.exec();
}
