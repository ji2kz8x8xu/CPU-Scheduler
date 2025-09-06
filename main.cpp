// 11120301 林桂
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <set>
#include <fstream>
#include <algorithm> // std::find
#include <unordered_map>
#include <map>

// Give the file a full name, add .txt
std::string processFileName(std::string& fileName) {
  if (fileName.size() < 4 || fileName.substr(fileName.size() - 4) != ".txt") {
    return fileName + ".txt";
  }
  return fileName;
}

struct PageInfo {
  int freq;    // 使用頻率
  int time;    // 時間戳記
};

class InputData {
 protected:
  std::string fileName;             // 檔案名稱
  std::vector<std::string> methods; // 方法名稱
  int pageFrameNum;                 // 記憶體頁框數量
  std::vector<int> pageReferences;  // page引用序列

 public:
  std::vector<std::string> getMethods() {
    return methods;
  }

  void loadMethods(int methodNum) {
    std::map<int, std::string> methodMap = {
      {1, "FIFO"},
      {2, "LRU"},
      {3, "Least Frequently Used Page Replacement"},
      {4, "Most Frequently Used Page Replacement "},
      {5, "Least Frequently Used LRU Page Replacement"}
    };

    if (methodNum == 6) {
      // 如果是 "ALL"，將所有方法推入
      for (auto& pair : methodMap) {
        methods.push_back(pair.second);
      }
    } else if (methodMap.find(methodNum) != methodMap.end()) {
      // 如果找到指定方法，加入到 vector 中
      methods.push_back(methodMap[methodNum]);
    } else {
      // 如果無效，加入 INVALID METHOD
      methods.push_back("INVALID METHOD");
    }
}

  void fromFile(const std::string& fileName) {
    this->fileName = fileName;
    std::ifstream file(fileName);
    if (!file.is_open()) {
      throw std::runtime_error("Cannot open file: " + fileName);
    }

    std::string line;
    // get first row
    getline(file, line);
    std::istringstream iss(line);
    std::string info;
    int methodNum;
    for (int i = 0; i < 2; i++) {
      getline(iss, info, ' ');
      if (i == 0) methodNum = std::stoi(info);
      else pageFrameNum = stoi(info);
    }

    loadMethods(methodNum);

    // get second row
    getline(file, line);

    std::istringstream iss2(line);
    for (char ch : line) {
      if (isdigit(ch)) {
      pageReferences.push_back(ch - '0');
      }
    }
    file.close();
  }

  void print() {
    std::cout << "Methods: ";
    for (const auto& method : methods) {
      std::cout << method << " ";
    }
    std::cout << "\nPage Frame Number: " << pageFrameNum << "\nPage References: ";
    if (pageReferences.empty()) {
      std::cout << "No page references found.";
    } else {
      for (int ref : pageReferences) {
        std::cout << ref << " ";
      }
    }
    std::cout << "\n";
  }
};


class PageReplacement : public InputData {
 private:
  int pageFaults = 0;                       // 要reference的不在記憶體
  int pageReplaces = 0;                     // 有從記憶體拿出再放入新的
  std::vector<std::vector<int>> pageFrames; // 記錄每一步 Page Frame 狀態
  std::vector<bool> pageFaultFlags;         // 記錄每一步是否發生 Page Fault
  
  // 重置變數
  void reset() { 
    pageFaults = 0;
    pageReplaces = 0;
    pageFrames.clear();
    pageFaultFlags.clear();

    pageFrames.resize(pageReferences.size()); // 根據頁面引用數量初始化
    pageFaultFlags.resize(pageReferences.size(), false); // 預設每一步都沒有 page fault
  }

  // 檢查 vector 中是否包含指定元素
  bool contains(std::vector<int>& vec, int value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
  }

  // bool contains(std::vector<PageInfo>& vec, int value) {
  //   return std::find_if(vec.begin(), vec.end(), [&](const PageInfo& p) {
  //       return p.page == value; // 比較 .page 是否等於 value
  //   }) != vec.end();
  // }

  bool contains(const std::map<int, PageInfo>& pageInfoMap, int page) {
      return pageInfoMap.find(page) != pageInfoMap.end();
  }

  int countUniqueReferences(std::vector<int>& pageReferences) {
    std::set<int> uniquePages(pageReferences.begin(), pageReferences.end());
    return uniquePages.size();
  }

  // 根據 freq 小的優先，time 小的優先，移除並回傳被刪除的 page
  int removePageWithFreqAndTime(std::map<int, PageInfo>& pageInfoMap, bool removeMin) {
    if (pageInfoMap.empty()) {
      std::cout << "pageInfoMap is empty, no task to remove.\n";
      return -1; // 表示沒有元素被刪除
    }

    // 初始化選擇的迭代器為第一個元素
    auto selected = pageInfoMap.begin();

    for (auto it = pageInfoMap.begin(); it != pageInfoMap.end(); ++it) {
      if (removeMin) {
        // freq 小的優先，time 小的優先
        if (it->second.freq < selected->second.freq || 
            (it->second.freq == selected->second.freq && it->second.time < selected->second.time)) {
          selected = it;
        }
      } else {
        // freq 大的優先，time 小的優先
        if (it->second.freq > selected->second.freq || 
          (it->second.freq == selected->second.freq && it->second.time < selected->second.time)) {
          selected = it;
        }
      }
    }

    int removedPage = selected->first; // 保存要刪除的 page
    pageInfoMap.erase(selected); // 移除條件符合的元素
    return removedPage; // 回傳被刪除的 page
  }

  void movePageToFirst(std::vector<int>& memory, int page) {
    // 把reference的放第一個
    auto refPtr = std::find(memory.begin(), memory.end(), page);
    // 取出此ref
    int refNum = *refPtr;
    // 移除
    memory.erase(refPtr);
    // 插到第一個
    memory.insert(memory.begin(), refNum);
  }

 public:

  // method 1
  void executeFIFO() {

    reset();

    std::queue<int> fifo;
    std::set<int> lookup; // 與queue同步 用來找有沒有在queue中
    for (int i = 0; i < pageReferences.size(); i++) {
      // 先檢查要reference的東西有沒有在queue裡面
      // if 有找到
      if (lookup.find(pageReferences.at(i)) != lookup.end()) {

      } else {
        pageFaults++;
        // 不在queue裡面 在判斷queue滿了沒 代表要reference的東西不在記憶體 pageFaults++;

        // 如果記憶體滿了
        if (fifo.size() == pageFrameNum) {
          int victim = fifo.front();
          fifo.pop();
          lookup.erase(victim);
          pageReplaces++;
        }

        // 把要用到的放入記憶體
        fifo.push(pageReferences.at(i));
        lookup.insert(pageReferences.at(i));
       
        // 有page fault 記錄下來
        pageFaultFlags.at(i) = true;
      }

      // 紀錄pageframe狀態
      std::queue<int> tempQueue = fifo;
      while (!tempQueue.empty()) {
        // 反向放到pageframe 因為reference的要放在前面
        pageFrames.at(i).insert(pageFrames.at(i).begin(), tempQueue.front());
        tempQueue.pop();
      }
      
    }
  }

  // Least Recently Used Page Replacement
  // method 2
  void executeLRU() {
    reset();

    // 先取得需要幾個timestamp
    int timeStampNum = countUniqueReferences(pageReferences);

    // 所有timestamp
    std::unordered_map<int, int> timeStamp;
    // 目前記憶體中的東西
    std::vector<int> memory;

    // i 同時也作為counter
    for (int i = 0; i < pageReferences.size(); i++) {
      // 如果已經在記憶體中
      if (contains(memory, pageReferences.at(i))) {
        // 更新timestamp 更新為目前的時間
        timeStamp[pageReferences.at(i)] = i;
        // 把reference的放第一個
        movePageToFirst(memory, pageReferences.at(i));
        
        // 不在記憶體中 要放入
      } else {
        pageFaults++;
        // 先檢查記憶體滿了嗎 滿了要先移除最少用的那個
        if (memory.size() == pageFrameNum) {
          // 找最少用的那個的key
          int lruPage = memory[0];
          int minTime = timeStamp[lruPage];
          for (int p : memory) {
            if (timeStamp[p] < minTime) {
              lruPage = p;
              minTime = timeStamp[p];
            }
          }

          // 從記憶體中移除 LRU 頁面
          memory.erase(std::remove(memory.begin(), memory.end(), lruPage), memory.end());
          timeStamp.erase(lruPage);
          pageReplaces++;
        }

        // 把要存取的加入memory reference放第一個
        memory.insert(memory.begin(), pageReferences.at(i));
        // 更新timestamp
        timeStamp[pageReferences.at(i)] = i;

        // 有page fault 記錄下來
        pageFaultFlags.at(i) = true;
      }

      // 紀錄pageframe狀態
      pageFrames.at(i) = memory;
      
    }
  }
  
  // least Frequnetly Used Page Replacement + FIFO
  // Freq 紀錄 Frequnetly timeStamp 紀錄FIFO
  // method 3
  void executeLFU_FIFO() {
    reset();
    // 目前記憶體中的東西 維護reference順序
    std::vector<int> memory;
    // memory的set 維護出去的順序
    std::map<int, PageInfo> memoryMap;


    // i 同時也作為counter
    for (int i = 0; i < pageReferences.size(); i++) {
      int page = pageReferences.at(i);
      // 如果已經在記憶體中 timestamp不變 因為是fifo
      if (contains(memoryMap, page)) {
        // 更新timestamp 更新為目前的時間
        // memoryMap[page].time = i;
        
        // 把reference的放第一個
        // movePageToFirst(memory, page);

        // 此page的使用次數freq+1
        memoryMap[page].freq = memoryMap[page].freq + 1;


        // 不在記憶體中 要放入
      } else {
        pageFaults++;
        // 先檢查記憶體滿了嗎 滿了要先移除Freq最少的
        if (memory.size() == pageFrameNum) {
          // 根據freq和time決定要移除誰
          int removePage = removePageWithFreqAndTime(memoryMap, true);
          // 更新目前記憶體
          memory.erase(std::remove(memory.begin(), memory.end(), removePage), memory.end());

          pageReplaces++;
        }

        // 把要存取的加入memory reference放第一個
        memory.insert(memory.begin(), page);
        // map也要更新 更新timestamp和初始化freq
        memoryMap[page].time = i;
        memoryMap[page].freq = 1;


        // 有page fault 記錄下來
        pageFaultFlags.at(i) = true;
      }

      // 紀錄pageframe狀態
      pageFrames.at(i) = memory;
      
    }
  }
  
  // Most Frequnetly Used Page Replacement + FIFO
  // Freq 紀錄 Frequnetly timeStamp 紀錄FIFO
  // method 4
  void executeMFU_FIFO() {
    reset();
    // 目前記憶體中的東西 維護reference順序
    std::vector<int> memory;
    // memory的set 維護出去的順序
    std::map<int, PageInfo> memoryMap;


    // i 同時也作為counter
    for (int i = 0; i < pageReferences.size(); i++) {
      int page = pageReferences.at(i);
      // 如果已經在記憶體中
      if (contains(memoryMap, page)) {
        // fifo timestamp不變
        // memoryMap[page].time = i;
        
        // reference後不用放第一個
        // movePageToFirst(memory, page);

        // 此page的使用次數freq+1
        memoryMap[page].freq = memoryMap[page].freq + 1;

        // 不在記憶體中 要放入
      } else {
        pageFaults++;
        // 先檢查記憶體滿了嗎 滿了要先移除Freq最少的
        if (memory.size() == pageFrameNum) {
          // 根據freq和time決定要移除誰
          int removePage = removePageWithFreqAndTime(memoryMap, false);
          // 更新目前記憶體
          memory.erase(std::remove(memory.begin(), memory.end(), removePage), memory.end());

          pageReplaces++;
        }

        // 把要存取的加入memory reference放第一個
        memory.insert(memory.begin(), page);
        // map也要更新 更新timestamp和初始化freq
        memoryMap[page].time = i;
        memoryMap[page].freq = 1;

        // 有page fault 記錄下來
        pageFaultFlags.at(i) = true;
      }

      // 紀錄pageframe狀態
      pageFrames.at(i) = memory;
      
    }
  }

  // LFU(Least Frequnetly Used Page Replacement)(freq) + 
  // LRU(Least Recently Used Page Replacement)(timestamp)
  // method 5
  void executeLFU_LRU() {
    reset();
    // 目前記憶體中的東西 維護reference順序
    std::vector<int> memory;
    // memory的set 維護出去的順序
    std::map<int, PageInfo> memoryMap;

    // i 同時也作為counter
    for (int i = 0; i < pageReferences.size(); i++) {
      int page = pageReferences.at(i);
      // 如果已經在記憶體中
      if (contains(memoryMap, page)) {
        // 更新timestamp 更新為目前的時間
        memoryMap[page].time = i;
        
        // 把reference的放第一個
        movePageToFirst(memory, page);

        // 此page的使用次數freq+1
        memoryMap[page].freq = memoryMap[page].freq + 1;

        // 不在記憶體中 要放入
      } else {
        pageFaults++;
        // 先檢查記憶體滿了嗎 滿了要先移除Freq最少的
        if (memory.size() == pageFrameNum) {
          // 根據freq和time決定要移除誰
          int removePage = removePageWithFreqAndTime(memoryMap, true);
          // 更新目前記憶體
          memory.erase(std::remove(memory.begin(), memory.end(), removePage), memory.end());
          pageReplaces++;
        }

        // 把要存取的加入memory reference放第一個
        memory.insert(memory.begin(), page);
        // map也要更新 更新timestamp和初始化freq
        memoryMap[page].time = i;
        memoryMap[page].freq = 1;

        // 有page fault 記錄下來
        pageFaultFlags.at(i) = true;
      }
      // 紀錄pageframe狀態
      pageFrames.at(i) = memory;
    }
  } 

  void printResult() {
    std::cout << "--------------FIFO-----------------------" << std::endl;
    for (int i = 0; i < pageReferences.size(); i++) {
      std::cout << pageReferences.at(i) << '\t';

      std::string pageFrameStr;
      for (auto& elem : pageFrames.at(i)) {
        pageFrameStr += std::to_string(elem);
      }
      std::cout << pageFrameStr << '\t';
      
      if (pageFaultFlags.at(i) == true) {
        std::cout << "F";
      }
      std::cout << std::endl;
    }

    std::cout << "Page Fault = " << pageFaults 
              << "  Page Replaces = " << pageReplaces 
              << "  Page Frames = " << pageFrameNum << std::endl;
  }

void writeToFile(const std::string& fileName, const std::string& methodName, bool isFirstMethod) {
    std::ios_base::openmode mode = (isFirstMethod) ? std::ios::trunc : std::ios::app;
    std::ofstream outputFile(fileName, mode);
    
    if (!outputFile.is_open()) {
        throw std::runtime_error("Cannot open output file: " + fileName);
    }
    
    outputFile << "--------------" << methodName << "-----------------------" << std::endl;
    for (int i = 0; i < pageReferences.size(); i++) {
        outputFile << pageReferences.at(i) << '\t';

        std::string pageFrameStr;
        for (auto& elem : pageFrames.at(i)) {
            pageFrameStr += std::to_string(elem);
        }
        outputFile << pageFrameStr;
        
        if (pageFaultFlags.at(i) == true) {
            outputFile  << '\t' << "F";
        }
        outputFile << std::endl;
    }

    outputFile << "Page Fault = " << pageFaults 
               << "  Page Replaces = " << pageReplaces 
               << "  Page Frames = " << pageFrameNum << std::endl;
    outputFile.close();
}

  void writeLineBreakToFile(const std::string& fileName) {
    std::ofstream outputFile(fileName, std::ios::app);
    outputFile << std::endl;
    outputFile.close();
  }
};

int main() {

  while (1) {
    std::string fileName;
    std::cout << "Please Enter File Name (eg. input1, input1.txt) : ";
    std::cin >> fileName;
    
    if (fileName == "exit") break;

    std::cout << std::endl;

    fileName = processFileName(fileName);

    try {
      PageReplacement Data;
      Data.fromFile(fileName);
      Data.print();

      bool isFirstMethod = true;

      std::vector<std::string> methods = Data.getMethods();
      for (int i = 0; i < methods.size(); i++) {
        std::string method = methods[i];
        
        if (method == "FIFO") {
            Data.executeFIFO();
        } else if (method == "LRU") {
            Data.executeLRU();
        } else if (method == "Least Frequently Used Page Replacement") {
            Data.executeLFU_FIFO();
        } else if (method == "Most Frequently Used Page Replacement ") {
            Data.executeMFU_FIFO();
        } else if (method == "Least Frequently Used LRU Page Replacement") {
            Data.executeLFU_LRU();
        } else if (method == "INVALID METHOD") {
          std::cout << "INVALID METHOD" << std::endl;
          break;
        }

        Data.writeToFile("out_" + fileName, method, isFirstMethod);

        isFirstMethod = false;
        
        // 如果不是最後一個方法，才換行
        if (i != methods.size() - 1) {
          Data.writeLineBreakToFile("out_" + fileName);
        }
      
        Data.printResult();
      }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }



  }
}