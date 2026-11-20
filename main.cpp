#include <iostream>
#include <string>
#include <vector>
#include <fstream>   // for reading and writing files
#include <sstream>   // for cutting a line at commas
#include <cctype>    // for tolower and isdigit
#include <algorithm> // for max, min and swap
using namespace std;

// One class (one row of the timetable)
struct ClassRecord {
    string course;
    string code;
    string teacher;
    string section;
    string room;
    string day;
    int startMin;   // minutes from midnight (09:00 = 540)
    int endMin;
};

// Prototype: tells C++ that hasClash exists (it is written lower in the file)
bool hasClash(const vector<ClassRecord>& timetable, const ClassRecord& c);

// "09:00" becomes 540
int timeToMinutes(string t) {
    int h = stoi(t.substr(0, 2));    // first 2 letters: hour
    int m = stoi(t.substr(3, 2));    // letters 4 and 5: minutes
    return h * 60 + m;
}

// Removes spaces (and tabs) from the start and end of a text
string trim(string s) {
    int first = s.find_first_not_of(" \t");    // position of the first real letter
    if (first == string::npos) {
        return "";                              // the text had only spaces
    }
    int last = s.find_last_not_of(" \t");      // position of the last real letter
    return s.substr(first, last - first + 1);
}

// Changes all letters to small letters. "Dr Ali" becomes "dr ali"
string toLower(string s) {
    for (int i = 0; i < s.size(); i++) {
        s[i] = tolower((unsigned char)s[i]);   // the cast makes special characters (like the micro sign) safe
    }
    return s;
}

// Changes any way of writing a day into our standard form.
// "mon", "MONDAY", " Monday " all become "Mon"
string normalizeDay(string d) {
    d = toLower(trim(d));
    if (d == "mon" || d == "monday")    return "Mon";
    if (d == "tue" || d == "tuesday")   return "Tue";
    if (d == "wed" || d == "wednesday") return "Wed";
    if (d == "thu" || d == "thursday")  return "Thu";
    if (d == "fri" || d == "friday")    return "Fri";
    if (d == "sat" || d == "saturday")  return "Sat";
    return d;    // not a day: stays as it is, and dayNumber will say it is invalid
}

// 540 becomes "09:00"
string minutesToTime(int m) {
    int h = m / 60;
    int mins = m % 60;
    string result = "";
    if (h < 10) result += "0";
    result += to_string(h) + ":";
    if (mins < 10) result += "0";
    result += to_string(mins);
    return result;
}

// Checks the time looks like HH:MM, example 09:30
bool isValidTime(string t) {
    if (t.size() != 5) return false;
    if (t[2] != ':') return false;
    if (!isdigit(t[0]) || !isdigit(t[1]) || !isdigit(t[3]) || !isdigit(t[4])) {
        return false;
    }
    int h = stoi(t.substr(0, 2));
    int m = stoi(t.substr(3, 2));
    return h < 24 && m < 60;
}

// Prints one class on one line
void printClass(const ClassRecord& c) {
    cout << c.course << " | " << c.teacher << " | " << c.section
         << " | " << c.room << " | " << c.day << " "
         << minutesToTime(c.startMin) << "-" << minutesToTime(c.endMin)
         << endl;
}

// Reads the CSV file and fills the vector.
// Bad rows are skipped with a warning. Returns false only if the file did not open.
bool loadFromCSV(const string& filename, vector<ClassRecord>& timetable) {
    ifstream file(filename);          // open the file
    if (!file.is_open()) {
        return false;                 // file not found
    }

    string line;
    int lineNumber = 1;
    getline(file, line);              // skip the first line (the headings)

    while (getline(file, line)) {     // read one line at a time
        lineNumber++;

        // Windows can leave a hidden '\r' at the end of the line. Remove it.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "") continue;     // skip empty lines

        stringstream ss(line);        // lets us cut the line at commas
        ClassRecord c;
        string start, end;

        getline(ss, c.course,  ',');
        getline(ss, c.code,    ',');
        getline(ss, c.teacher, ',');
        getline(ss, c.section, ',');
        getline(ss, c.room,    ',');
        getline(ss, c.day,     ',');
        getline(ss, start,     ',');
        getline(ss, end,       ',');

        // Clean the spaces from every field
        c.course  = trim(c.course);
        c.code    = trim(c.code);
        c.teacher = trim(c.teacher);
        c.section = trim(c.section);
        c.room    = trim(c.room);
        c.day     = normalizeDay(c.day);
        start     = trim(start);
        end       = trim(end);

        // Check the row BEFORE using it
        if (c.course == "" || c.teacher == "" || c.section == "" ||
            c.room == "" || c.day == "" ||
            !isValidTime(start) || !isValidTime(end)) {
            cout << "Warning: skipped bad row at line " << lineNumber
                 << ": " << line << endl;
            continue;                 // go to the next line
        }

        c.startMin = timeToMinutes(start);
        c.endMin   = timeToMinutes(end);

        if (c.startMin >= c.endMin) {
            cout << "Warning: skipped row at line " << lineNumber
                 << " (end time is not after start time): " << line << endl;
            continue;
        }

        timetable.push_back(c);       // add to our vector
    }

    file.close();
    return true;
}

// Gives back the field we want to search in.
// 1 = section, 2 = teacher, 3 = course, 4 = room, 5 = day
string getField(const ClassRecord& c, int choice) {
    if (choice == 1) return c.section;
    if (choice == 2) return c.teacher;
    if (choice == 3) return c.course;
    if (choice == 4) return c.room;
    return c.day;
}

// Linear search on any field. Prints matches, returns how many were found.
int searchByField(const vector<ClassRecord>& timetable, int choice, string wanted) {
    int found = 0;
    wanted = toLower(wanted);

    for (int i = 0; i < timetable.size(); i++) {
        string value = toLower(getField(timetable[i], choice));

        if (value.find(wanted) != string::npos) {   // is the wanted text inside?
            printClass(timetable[i]);
            found++;
        }
    }
    return found;
}

// Gives a number to each day so we can compare days
int dayNumber(string day) {
    if (day == "Mon") return 1;
    if (day == "Tue") return 2;
    if (day == "Wed") return 3;
    if (day == "Thu") return 4;
    if (day == "Fri") return 5;
    if (day == "Sat") return 6;
    return 99;   // unknown day goes to the end
}

// Returns true if class a should come BEFORE class b
// First compare the day. If the day is the same, compare the start time.
bool isBefore(const ClassRecord& a, const ClassRecord& b) {
    if (dayNumber(a.day) != dayNumber(b.day)) {
        return dayNumber(a.day) < dayNumber(b.day);
    }
    return a.startMin < b.startMin;
}

// Bubble sort: sorts the timetable by day, then by start time
void bubbleSort(vector<ClassRecord>& timetable) {
    int n = timetable.size();

    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;   // did we swap anything in this round?

        for (int j = 0; j < n - 1 - i; j++) {
            // if the next class should come first, swap the two
            if (isBefore(timetable[j + 1], timetable[j])) {
                ClassRecord temp = timetable[j];
                timetable[j] = timetable[j + 1];
                timetable[j + 1] = temp;
                swapped = true;
            }
        }

        if (!swapped) break;    // nothing swapped = already sorted, stop early
    }
}

// Prints all classes in the vector
void printAll(const vector<ClassRecord>& timetable) {
    if (timetable.size() == 0) {
        cout << "Timetable is empty." << endl;
        return;
    }
    for (int i = 0; i < timetable.size(); i++) {
        printClass(timetable[i]);
    }
}

// Asks the user what to search, then calls searchByField
void searchMenu(const vector<ClassRecord>& timetable) {
    int choice;
    cout << "Search by: 1.Section 2.Teacher 3.Course 4.Room 5.Day" << endl;
    cout << "Enter choice: ";
    cin >> choice;

    if (cin.fail() || choice < 1 || choice > 5) {
        cin.clear();                 // clear the error
        cin.ignore(1000, '\n');      // throw away the wrong input
        cout << "Invalid choice." << endl;
        return;
    }

    cin.ignore();                    // remove the Enter key
    string wanted;
    cout << "Enter text to search: ";
    getline(cin, wanted);

    cout << "-------------------------" << endl;
    int count = searchByField(timetable, choice, wanted);

    if (count == 0) {
        cout << "No classes found for " << wanted << endl;
    } else {
        cout << count << " class(es) found." << endl;
    }
}

// Text must not be empty and must not have a comma (a comma would break our CSV)
bool isGoodText(string s) {
    return s != "" && s.find(',') == string::npos;
}

// Writes the whole vector into the CSV file. Returns true if it worked.
bool saveToCSV(const string& filename, const vector<ClassRecord>& timetable) {
    ofstream file(filename);          // opens the file and erases the old content
    if (!file.is_open()) {
        return false;
    }

    file << "Course,Code,Teacher,Section,Room,Day,Start,End" << endl;   // headings

    for (int i = 0; i < timetable.size(); i++) {
        const ClassRecord& c = timetable[i];
        file << c.course << "," << c.code << "," << c.teacher << ","
             << c.section << "," << c.room << "," << c.day << ","
             << minutesToTime(c.startMin) << "," << minutesToTime(c.endMin)
             << endl;
    }

    file.close();
    return true;
}

// Asks the user for a new class and adds it to the vector
void addClass(vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');           // remove the Enter key left from the menu

    ClassRecord c;
    string start, end;

    cout << "Course name: ";                getline(cin, c.course);
    cout << "Course code: ";                getline(cin, c.code);
    cout << "Teacher: ";                    getline(cin, c.teacher);
    cout << "Section (example BAI-3B): ";   getline(cin, c.section);
    cout << "Room: ";                       getline(cin, c.room);
    cout << "Day (Mon, Tue, Wed, Thu, Fri, Sat): "; getline(cin, c.day);
    cout << "Start time (HH:MM): ";         getline(cin, start);
    cout << "End time (HH:MM): ";           getline(cin, end);

    c.day = normalizeDay(c.day);     // accept mon, MONDAY, etc.

    // Check everything before adding
    if (!isGoodText(c.course) || !isGoodText(c.code) || !isGoodText(c.teacher) ||
        !isGoodText(c.section) || !isGoodText(c.room)) {
        cout << "Error: fields cannot be empty or have commas." << endl;
        return;
    }
    if (dayNumber(c.day) == 99) {
        cout << "Error: day must be Mon, Tue, Wed, Thu, Fri or Sat." << endl;
        return;
    }
    if (!isValidTime(start) || !isValidTime(end)) {
        cout << "Error: time must look like 09:30." << endl;
        return;
    }

    c.startMin = timeToMinutes(start);
    c.endMin   = timeToMinutes(end);

    if (c.startMin >= c.endMin) {
        cout << "Error: end time must be after start time." << endl;
        return;
    }

    if (hasClash(timetable, c)) {
        char answer;
        cout << "This class clashes. Add anyway? (y/n): ";
        cin >> answer;
        if (answer != 'y' && answer != 'Y') {
            cout << "Not added." << endl;
            return;
        }
    }

    timetable.push_back(c);
    cout << "Class added. (Choose Save to keep it in the file.)" << endl;
}

// Shows all classes with numbers, then removes the one the user picks
void removeClass(vector<ClassRecord>& timetable) {
    if (timetable.size() == 0) {
        cout << "Timetable is empty. Nothing to remove." << endl;
        return;
    }

    // Show the classes with numbers (1, 2, 3 ...)
    for (int i = 0; i < timetable.size(); i++) {
        cout << i + 1 << ". ";
        printClass(timetable[i]);
    }

    int number;
    cout << "Enter the number to remove (0 to cancel): ";
    cin >> number;

    if (cin.fail()) {                 // the user typed letters
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Invalid input." << endl;
        return;
    }

    if (number == 0) {
        cout << "Cancelled." << endl;
        return;
    }

    if (number < 1 || number > timetable.size()) {
        cout << "Error: no class with that number." << endl;
        return;
    }

    cout << "Removing: ";
    printClass(timetable[number - 1]);          // numbers start at 1, index starts at 0

    timetable.erase(timetable.begin() + (number - 1));
    cout << "Class removed. (Choose Save to keep the change in the file.)" << endl;
}

// Do two classes overlap? (same day AND time ranges cross each other)
bool isOverlap(const ClassRecord& a, const ClassRecord& b) {
    if (a.day != b.day) return false;
    return a.startMin < b.endMin && b.startMin < a.endMin;
}

// Same course, teacher, room and time for two sections = one shared class (not a clash)
bool isSharedClass(const ClassRecord& a, const ClassRecord& b) {
    return a.course == b.course && a.teacher == b.teacher && a.room == b.room &&
           a.startMin == b.startMin && a.endMin == b.endMin;
}

// Finds clashes for one type: 1 = section, 2 = teacher, 4 = room.
// Prints every clash and returns how many were found.
int findClashes(const vector<ClassRecord>& timetable, int choice) {
    string label = "ROOM";
    if (choice == 1) label = "SECTION";
    if (choice == 2) label = "TEACHER";

    int count = 0;

    for (int i = 0; i < timetable.size(); i++) {
        for (int j = i + 1; j < timetable.size(); j++) {

            // same room/teacher/section AND overlapping time = clash
            if (getField(timetable[i], choice) == getField(timetable[j], choice)
                && isOverlap(timetable[i], timetable[j])
                && !isSharedClass(timetable[i], timetable[j])) {

                // the overlapping part is from the later start to the earlier end
                int overlapStart = max(timetable[i].startMin, timetable[j].startMin);
                int overlapEnd   = min(timetable[i].endMin, timetable[j].endMin);

                cout << label << " CLASH (" << getField(timetable[i], choice) << ") on "
                     << timetable[i].day << ", overlap "
                     << minutesToTime(overlapStart) << "-" << minutesToTime(overlapEnd) << endl;
                cout << "   - ";
                printClass(timetable[i]);
                cout << "   - ";
                printClass(timetable[j]);

                count++;
            }
        }
    }
    return count;
}

// Runs all three checks and prints a report
void clashReport(const vector<ClassRecord>& timetable) {
    cout << "----- Clash Report -----" << endl;
    int total = 0;
    total += findClashes(timetable, 4);   // rooms
    total += findClashes(timetable, 2);   // teachers
    total += findClashes(timetable, 1);   // sections

    if (total == 0) {
        cout << "No clashes found. The timetable is clean!" << endl;
    } else {
        cout << "Total clashes: " << total << endl;
    }
}

// Checks a NEW class against all existing classes.
// Prints a warning for every clash. Returns true if at least one clash exists.
bool hasClash(const vector<ClassRecord>& timetable, const ClassRecord& c) {
    bool clash = false;
    for (int i = 0; i < timetable.size(); i++) {
        if (!isOverlap(c, timetable[i])) continue;

        if (c.room == timetable[i].room) {
            cout << "WARNING: room " << c.room << " is already used by: ";
            printClass(timetable[i]);
            clash = true;
        }
        if (c.teacher == timetable[i].teacher) {
            cout << "WARNING: teacher " << c.teacher << " is already teaching: ";
            printClass(timetable[i]);
            clash = true;
        }
        if (c.section == timetable[i].section) {
            cout << "WARNING: section " << c.section << " already has: ";
            printClass(timetable[i]);
            clash = true;
        }
    }
    return clash;
}

// Free classroom finder: the user gives a day and a time range.
void freeRooms(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    string day, start, end;
    cout << "Day: ";                getline(cin, day);
    cout << "Start time (HH:MM): "; getline(cin, start);
    cout << "End time (HH:MM): ";   getline(cin, end);

    day = normalizeDay(day);
    if (dayNumber(day) == 99 || !isValidTime(start) || !isValidTime(end)) {
        cout << "Error: wrong day or time." << endl;
        return;
    }

    ClassRecord q;                  // the time slot we are asking about
    q.day = day;
    q.startMin = timeToMinutes(start);
    q.endMin = timeToMinutes(end);
    if (q.startMin >= q.endMin) {
        cout << "Error: end time must be after start time." << endl;
        return;
    }

    // Step 1: make a list of all different rooms (no repeats)
    vector<string> rooms;
    for (int i = 0; i < timetable.size(); i++) {
        bool found = false;
        for (int j = 0; j < rooms.size(); j++) {
            if (rooms[j] == timetable[i].room) found = true;
        }
        if (!found) rooms.push_back(timetable[i].room);
    }

    // Step 2: for each room, check if any class overlaps the slot
    cout << "----- " << day << " " << start << "-" << end << " -----" << endl;
    int freeCount = 0;
    for (int r = 0; r < rooms.size(); r++) {
        bool busy = false;
        int busyIndex = -1;
        for (int i = 0; i < timetable.size(); i++) {
            if (timetable[i].room == rooms[r] && isOverlap(q, timetable[i])) {
                busy = true;
                busyIndex = i;
            }
        }
        if (busy) {
            cout << "OCCUPIED: " << rooms[r] << "  -> ";
            printClass(timetable[busyIndex]);
        } else {
            cout << "FREE:     " << rooms[r] << endl;
            freeCount++;
        }
    }
    cout << freeCount << " room(s) free." << endl;
}

// ---------- SORTING ----------

// Compares two classes. key: 1 = day and time, 2 = course, 3 = teacher.
// Returns true if a should come BEFORE b.
bool isBeforeBy(const ClassRecord& a, const ClassRecord& b, int key) {
    if (key == 2) {
        string x = toLower(a.course), y = toLower(b.course);
        if (x != y) return x < y;
        return isBefore(a, b);          // same course: order by day and time
    }
    if (key == 3) {
        string x = toLower(a.teacher), y = toLower(b.teacher);
        if (x != y) return x < y;
        return isBefore(a, b);
    }
    return isBefore(a, b);              // key 1: day, then time
}

// 1. Bubble sort: swap neighbours that are in the wrong order
void bubbleSortBy(vector<ClassRecord>& t, int key) {
    int n = t.size();
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; j++) {
            if (isBeforeBy(t[j + 1], t[j], key)) {
                swap(t[j], t[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

// 2. Insertion sort: take one item, insert it in the right place on its left
void insertionSortBy(vector<ClassRecord>& t, int key) {
    int n = t.size();
    for (int i = 1; i < n; i++) {
        ClassRecord current = t[i];
        int j = i - 1;
        while (j >= 0 && isBeforeBy(current, t[j], key)) {
            t[j + 1] = t[j];            // move the bigger item one step right
            j--;
        }
        t[j + 1] = current;             // put current in the empty place
    }
}

// 3. Selection sort: find the smallest item and put it at the front, repeat
void selectionSortBy(vector<ClassRecord>& t, int key) {
    int n = t.size();
    for (int i = 0; i < n - 1; i++) {
        int smallest = i;
        for (int j = i + 1; j < n; j++) {
            if (isBeforeBy(t[j], t[smallest], key)) smallest = j;
        }
        if (smallest != i) swap(t[i], t[smallest]);
    }
}

// 4. Merge sort (recursion). Merges two sorted parts: left..mid and mid+1..right
void mergeParts(vector<ClassRecord>& t, int left, int mid, int right, int key) {
    vector<ClassRecord> temp;
    int i = left, j = mid + 1;
    while (i <= mid && j <= right) {
        if (isBeforeBy(t[j], t[i], key)) temp.push_back(t[j++]);
        else                             temp.push_back(t[i++]);
    }
    while (i <= mid)   temp.push_back(t[i++]);
    while (j <= right) temp.push_back(t[j++]);
    for (int k = 0; k < temp.size(); k++) t[left + k] = temp[k];
}

void mergeSortRec(vector<ClassRecord>& t, int left, int right, int key) {
    if (left >= right) return;                 // base case: 0 or 1 item is sorted
    int mid = (left + right) / 2;
    mergeSortRec(t, left, mid, key);           // sort the left half
    mergeSortRec(t, mid + 1, right, key);      // sort the right half
    mergeParts(t, left, mid, right, key);      // merge the two sorted halves
}

void mergeSortBy(vector<ClassRecord>& t, int key) {
    if (t.size() < 2) return;
    mergeSortRec(t, 0, t.size() - 1, key);
}

// 5. Quick sort (recursion). Last item is the pivot.
int partitionBy(vector<ClassRecord>& t, int low, int high, int key) {
    ClassRecord pivot = t[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (isBeforeBy(t[j], pivot, key)) {    // smaller than pivot: move to the left side
            i++;
            swap(t[i], t[j]);
        }
    }
    swap(t[i + 1], t[high]);                   // put the pivot in its final place
    return i + 1;
}

void quickSortRec(vector<ClassRecord>& t, int low, int high, int key) {
    if (low < high) {                          // base case: low >= high means done
        int p = partitionBy(t, low, high, key);
        quickSortRec(t, low, p - 1, key);      // sort the left of the pivot
        quickSortRec(t, p + 1, high, key);     // sort the right of the pivot
    }
}

void quickSortBy(vector<ClassRecord>& t, int key) {
    if (t.size() < 2) return;
    quickSortRec(t, 0, t.size() - 1, key);
}

// Asks the user how to sort, then sorts the real timetable
void sortMenu(vector<ClassRecord>& timetable) {
    int key, algo;
    cout << "Sort by: 1.Day and time  2.Course  3.Teacher" << endl;
    cout << "Enter choice: ";
    cin >> key;
    cout << "Algorithm: 1.Bubble 2.Insertion 3.Selection 4.Merge 5.Quick" << endl;
    cout << "Enter choice: ";
    cin >> algo;

    if (cin.fail() || key < 1 || key > 3 || algo < 1 || algo > 5) {
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Invalid choice." << endl;
        return;
    }

    if (algo == 1) bubbleSortBy(timetable, key);
    else if (algo == 2) insertionSortBy(timetable, key);
    else if (algo == 3) selectionSortBy(timetable, key);
    else if (algo == 4) mergeSortBy(timetable, key);
    else quickSortBy(timetable, key);

    cout << "Sorted. Showing the result:" << endl;
    printAll(timetable);
}

// ---------- BINARY SEARCH ----------

// Recursive binary search on data sorted by course name.
// Returns the index of a match, or -1. "steps" counts how many checks we did.
int binarySearchRec(const vector<ClassRecord>& t, int low, int high,
                    const string& target, int& steps) {
    if (low > high) return -1;                 // base case: nothing left to check
    steps++;
    int mid = (low + high) / 2;
    string value = toLower(t[mid].course);

    if (value == target) return mid;           // found it
    if (target < value) return binarySearchRec(t, low, mid - 1, target, steps);   // go left
    return binarySearchRec(t, mid + 1, high, target, steps);                      // go right
}

void binarySearchMenu(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    string name;
    cout << "Enter the exact course name: ";
    getline(cin, name);
    string target = toLower(trim(name));

    // Binary search needs sorted data, so we sort a COPY by course first
    vector<ClassRecord> sorted = timetable;
    mergeSortBy(sorted, 2);

    int steps = 0;
    int n = sorted.size();
    int index = binarySearchRec(sorted, 0, n - 1, target, steps);

    if (index == -1) {
        cout << "Course not found. (Binary search used " << steps << " checks.)" << endl;
        return;
    }

    // The same course can appear many times (different sections). Show all of them.
    int left = index, right = index;
    while (left > 0 && toLower(sorted[left - 1].course) == target) left--;
    while (right + 1 < n && toLower(sorted[right + 1].course) == target) right++;

    for (int i = left; i <= right; i++) printClass(sorted[i]);
    cout << (right - left + 1) << " class(es) found. Binary search used "
         << steps << " checks (linear search could need up to " << n << ")." << endl;
}

// ---------- UNDO STACK (LIFO) ----------

// One change we can undo.
// type 1 = a class was ADDED, 2 = a class was REMOVED, 3 = a class was CHANGED
struct Action {
    int type;
    int index;           // position in the vector
    ClassRecord record;  // added: the new class, removed: the removed class, changed: the OLD class
};

struct ActionNode {
    Action data;
    ActionNode* next;    // pointer to the node below
};

class ActionStack {
private:
    ActionNode* top;     // the newest action
    int count;
public:
    ActionStack() { top = nullptr; count = 0; }

    ~ActionStack() { clear(); }                                 // free all nodes
    ActionStack(const ActionStack&) = delete;                   // no copying (Rule of Three)
    ActionStack& operator=(const ActionStack&) = delete;

    bool isEmpty() const { return top == nullptr; }
    int size() const { return count; }

    void push(const Action& a) {
        ActionNode* n = new ActionNode;   // make a new node
        n->data = a;
        n->next = top;                    // new node points to the old top
        top = n;                          // new node becomes the top
        count++;
    }

    bool pop(Action& out) {
        if (top == nullptr) return false; // empty stack
        ActionNode* n = top;
        out = n->data;
        top = n->next;                    // the node below becomes the top
        delete n;                         // free the memory
        count--;
        return true;
    }

    void clear() {
        Action a;
        while (pop(a)) {}
    }
};

// Are two classes exactly the same?
bool sameClass(const ClassRecord& a, const ClassRecord& b) {
    return a.course == b.course && a.code == b.code && a.teacher == b.teacher &&
           a.section == b.section && a.room == b.room && a.day == b.day &&
           a.startMin == b.startMin && a.endMin == b.endMin;
}

// After "remove class": compare the old and new vector to find what was removed
void recordRemoval(const vector<ClassRecord>& before, const vector<ClassRecord>& after,
                   ActionStack& undo) {
    if (after.size() + 1 != before.size()) return;     // nothing was removed
    int i = 0;
    while (i < after.size() && sameClass(before[i], after[i])) i++;
    Action a;
    a.type = 2;
    a.index = i;
    a.record = before[i];
    undo.push(a);
}

// Undo the last change
void undoLast(vector<ClassRecord>& t, ActionStack& undo) {
    Action a;
    if (!undo.pop(a)) {
        cout << "Nothing to undo." << endl;
        return;
    }

    if (a.type == 1 && a.index < t.size()) {                    // undo an ADD
        cout << "Undo: removed the added class: ";
        printClass(t[a.index]);
        t.erase(t.begin() + a.index);
    } else if (a.type == 2 && a.index <= t.size()) {            // undo a REMOVE
        t.insert(t.begin() + a.index, a.record);
        cout << "Undo: brought back: ";
        printClass(a.record);
    } else if (a.type == 3 && a.index < t.size()) {             // undo a CHANGE
        t[a.index] = a.record;
        cout << "Undo: restored: ";
        printClass(a.record);
    } else {
        cout << "Could not undo this change." << endl;
    }
}

// ---------- REQUEST QUEUE (FIFO) ----------

struct ChangeRequest {
    string course;
    string section;
    string newRoom;
};

struct RequestNode {
    ChangeRequest data;
    RequestNode* next;
};

class RequestQueue {
private:
    RequestNode* front;   // the oldest request (leaves first)
    RequestNode* rear;    // the newest request (joins here)
    int count;
public:
    RequestQueue() { front = nullptr; rear = nullptr; count = 0; }

    ~RequestQueue() { ChangeRequest r; while (dequeue(r)) {} }
    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    bool isEmpty() const { return front == nullptr; }
    int size() const { return count; }

    void enqueue(const ChangeRequest& r) {
        RequestNode* n = new RequestNode;
        n->data = r;
        n->next = nullptr;
        if (rear == nullptr) {            // queue was empty
            front = n;
            rear = n;
        } else {
            rear->next = n;               // join at the back
            rear = n;
        }
        count++;
    }

    bool dequeue(ChangeRequest& out) {
        if (front == nullptr) return false;
        RequestNode* n = front;
        out = n->data;
        front = n->next;                  // the next request is now first
        if (front == nullptr) rear = nullptr;
        delete n;
        count--;
        return true;
    }
};

// Handles all waiting requests, oldest first. Each change can be undone.
void processRequests(vector<ClassRecord>& t, RequestQueue& q, ActionStack& undo) {
    if (q.isEmpty()) {
        cout << "No requests waiting." << endl;
        return;
    }

    ChangeRequest r;
    while (q.dequeue(r)) {
        int changed = 0;
        for (int i = 0; i < t.size(); i++) {
            if (toLower(t[i].course) == toLower(r.course) &&
                toLower(t[i].section) == toLower(r.section)) {

                // save the OLD class for undo
                Action a;
                a.type = 3;
                a.index = i;
                a.record = t[i];
                undo.push(a);

                t[i].room = r.newRoom;                   // apply the change

                // warn if the new room is already used at that time
                for (int k = 0; k < t.size(); k++) {
                    if (k != i && t[k].room == t[i].room && isOverlap(t[i], t[k])) {
                        cout << "   WARNING: room clash with: ";
                        printClass(t[k]);
                    }
                }
                cout << "Changed: ";
                printClass(t[i]);
                changed++;
            }
        }
        if (changed == 0) {
            cout << "No class found for " << r.course << " / " << r.section << endl;
        }
    }
}

void requestMenu(vector<ClassRecord>& t, RequestQueue& q, ActionStack& undo) {
    int ch = -1;
    while (ch != 0) {
        cout << endl << "--- Change requests (" << q.size() << " waiting) ---" << endl;
        cout << "1. Add a room-change request" << endl;
        cout << "2. Process all requests (oldest first)" << endl;
        cout << "0. Back" << endl;
        cout << "Enter choice: ";
        cin >> ch;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            ch = -1;
            cout << "Please enter a number." << endl;
            continue;
        }

        if (ch == 1) {
            cin.ignore(1000, '\n');
            ChangeRequest r;
            cout << "Course name: ";  getline(cin, r.course);
            cout << "Section: ";      getline(cin, r.section);
            cout << "New room: ";     getline(cin, r.newRoom);
            r.course = trim(r.course);
            r.section = trim(r.section);
            r.newRoom = trim(r.newRoom);

            if (!isGoodText(r.course) || !isGoodText(r.section) || !isGoodText(r.newRoom)) {
                cout << "Error: fields cannot be empty or have commas." << endl;
            } else {
                q.enqueue(r);
                cout << "Request added to the queue." << endl;
            }
        } else if (ch == 2) {
            processRequests(t, q, undo);
        }
    }
}

// Reads one number safely. Returns false if the user typed letters.
bool readInt(int& x) {
    cin >> x;
    if (cin.fail()) {
        cin.clear();
        cin.ignore(1000, '\n');
        return false;
    }
    cin.ignore(1000, '\n');      // remove the Enter key after the number
    return true;
}

// ================= 1. SINGLY LINKED LIST: My Timetable =================

struct ClassNode {
    ClassRecord data;
    ClassNode* next;             // points to the next class in my timetable
};

class MyTimetable {
private:
    ClassNode* head;             // first node
    int count;
public:
    MyTimetable() { head = nullptr; count = 0; }
    ~MyTimetable() { clear(); }                               // free all nodes
    MyTimetable(const MyTimetable&) = delete;                 // no copying (Rule of Three)
    MyTimetable& operator=(const MyTimetable&) = delete;

    int size() const { return count; }

    // INSERTION at the end
    void addEnd(const ClassRecord& c) {
        ClassNode* n = new ClassNode;
        n->data = c;
        n->next = nullptr;
        if (head == nullptr) {
            head = n;                                         // first node
        } else {
            ClassNode* cur = head;
            while (cur->next != nullptr) cur = cur->next;     // walk to the last node
            cur->next = n;
        }
        count++;
    }

    // DELETION by position (1 = first)
    bool removeAt(int pos) {
        if (pos < 1 || pos > count) return false;
        ClassNode* toDelete;
        if (pos == 1) {
            toDelete = head;
            head = head->next;                                // head moves forward
        } else {
            ClassNode* prev = head;
            for (int i = 1; i < pos - 1; i++) prev = prev->next;
            toDelete = prev->next;
            prev->next = toDelete->next;                      // skip the deleted node
        }
        delete toDelete;
        count--;
        return true;
    }

    // TRAVERSAL: print all classes with numbers
    void printAll() const {
        if (head == nullptr) {
            cout << "Your timetable is empty." << endl;
            return;
        }
        int number = 1;
        for (const ClassNode* cur = head; cur != nullptr; cur = cur->next) {
            cout << number++ << ". ";
            printClass(cur->data);
        }
    }

    // SEARCH by course name (part of the name is enough). Returns how many were found.
    int searchCourse(string name) const {
        name = toLower(trim(name));
        int found = 0;
        for (const ClassNode* cur = head; cur != nullptr; cur = cur->next) {
            if (toLower(cur->data.course).find(name) != string::npos) {
                printClass(cur->data);
                found++;
            }
        }
        return found;
    }

    // Does a new class overlap with any class in my timetable? (personal clash)
    bool clashesWith(const ClassRecord& c) const {
        bool clash = false;
        for (const ClassNode* cur = head; cur != nullptr; cur = cur->next) {
            if (isOverlap(c, cur->data)) {
                cout << "CLASH with: ";
                printClass(cur->data);
                clash = true;
            }
        }
        return clash;
    }

    // Copies the list into a vector (so we can reuse saveToCSV)
    void toVector(vector<ClassRecord>& v) const {
        for (const ClassNode* cur = head; cur != nullptr; cur = cur->next) {
            v.push_back(cur->data);
        }
    }

    void clear() {
        while (head != nullptr) {
            ClassNode* n = head;
            head = head->next;
            delete n;
        }
        count = 0;
    }
};

void myTimetableMenu(const vector<ClassRecord>& timetable, MyTimetable& my) {
    cin.ignore(1000, '\n');
    int ch = -1;
    while (ch != 0) {
        cout << endl << "--- My Timetable (" << my.size() << " classes) ---" << endl;
        cout << "1. Add a class from the university timetable" << endl;
        cout << "2. Remove a class" << endl;
        cout << "3. Show my timetable" << endl;
        cout << "4. Search in my timetable" << endl;
        cout << "5. Save my timetable (my_timetable.csv)" << endl;
        cout << "0. Back" << endl;
        cout << "Enter choice: ";
        if (!readInt(ch)) { ch = -1; cout << "Please enter a number." << endl; continue; }

        if (ch == 1) {
            string name;
            cout << "Course name (or part of it): ";
            getline(cin, name);
            name = toLower(trim(name));

            vector<int> found;                       // positions of matching classes
            for (int i = 0; i < timetable.size(); i++) {
                if (toLower(timetable[i].course).find(name) != string::npos) found.push_back(i);
            }
            if (name == "" || found.empty()) {
                cout << "No matching course." << endl;
                continue;
            }
            for (int k = 0; k < found.size(); k++) {
                cout << k + 1 << ". ";
                printClass(timetable[found[k]]);
            }
            cout << "Pick a number (0 to cancel): ";
            int pick;
            if (!readInt(pick) || pick < 1 || pick > (int)found.size()) {
                cout << "Cancelled." << endl;
                continue;
            }
            const ClassRecord& chosen = timetable[found[pick - 1]];
            if (my.clashesWith(chosen)) {
                cout << "Not added because of the clash. Try another section." << endl;
            } else {
                my.addEnd(chosen);
                cout << "Added to your timetable." << endl;
            }
        } else if (ch == 2) {
            my.printAll();
            if (my.size() > 0) {
                cout << "Number to remove (0 to cancel): ";
                int n;
                if (readInt(n) && my.removeAt(n)) cout << "Removed." << endl;
                else cout << "Nothing removed." << endl;
            }
        } else if (ch == 3) {
            my.printAll();
        } else if (ch == 4) {
            string name;
            cout << "Course to find: ";
            getline(cin, name);
            if (my.searchCourse(name) == 0) cout << "Not found in your timetable." << endl;
        } else if (ch == 5) {
            vector<ClassRecord> v;
            my.toVector(v);
            if (saveToCSV("my_timetable.csv", v)) cout << "Saved." << endl;
            else cout << "Error: could not save." << endl;
        }
    }
}

// ================= 2. DOUBLY LINKED LIST: Class browser =================

struct DNode {
    ClassRecord data;
    DNode* prev;                 // the class before
    DNode* next;                 // the class after
};

class ClassBrowser {
private:
    DNode* head;
    DNode* tail;
    int count;
public:
    ClassBrowser() { head = nullptr; tail = nullptr; count = 0; }
    ~ClassBrowser() {
        while (head != nullptr) {
            DNode* n = head;
            head = head->next;
            delete n;
        }
    }
    ClassBrowser(const ClassBrowser&) = delete;
    ClassBrowser& operator=(const ClassBrowser&) = delete;

    DNode* getHead() const { return head; }

    // INSERTION at the end (O(1) because we keep the tail)
    void insertEnd(const ClassRecord& c) {
        DNode* n = new DNode;
        n->data = c;
        n->next = nullptr;
        n->prev = tail;
        if (tail == nullptr) head = n;     // list was empty
        else tail->next = n;
        tail = n;
        count++;
    }

    // DELETION of one node (O(1): we know its neighbours)
    void deleteNode(DNode* node) {
        if (node->prev != nullptr) node->prev->next = node->next;
        else head = node->next;            // we deleted the first node
        if (node->next != nullptr) node->next->prev = node->prev;
        else tail = node->prev;            // we deleted the last node
        delete node;
        count--;
    }

    // FORWARD traversal
    void printForward() const {
        for (const DNode* cur = head; cur != nullptr; cur = cur->next) printClass(cur->data);
    }

    // BACKWARD traversal
    void printBackward() const {
        for (const DNode* cur = tail; cur != nullptr; cur = cur->prev) printClass(cur->data);
    }
};

void browseMenu(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    if (timetable.empty()) {
        cout << "Timetable is empty." << endl;
        return;
    }

    vector<ClassRecord> sorted = timetable;
    mergeSortBy(sorted, 1);                           // order by day and time
    ClassBrowser list;
    for (int i = 0; i < sorted.size(); i++) list.insertEnd(sorted[i]);

    DNode* current = list.getHead();
    string cmd;
    while (current != nullptr) {
        cout << endl << "Now: ";
        printClass(current->data);
        cout << "n=next  p=previous  d=hide this class  f=all forward  b=all backward  0=exit: ";
        cin >> cmd;
        cin.ignore(1000, '\n');

        if (cmd == "n") {
            if (current->next != nullptr) current = current->next;
            else cout << "This is the last class." << endl;
        } else if (cmd == "p") {
            if (current->prev != nullptr) current = current->prev;
            else cout << "This is the first class." << endl;
        } else if (cmd == "d") {
            DNode* keep = (current->next != nullptr) ? current->next : current->prev;
            list.deleteNode(current);                 // hides it from this browser only
            current = keep;
            if (current == nullptr) cout << "No classes left." << endl;
        } else if (cmd == "f") {
            list.printForward();
        } else if (cmd == "b") {
            list.printBackward();
        } else if (cmd == "0") {
            break;
        } else {
            cout << "Unknown command." << endl;
        }
    }
}

// ================= 3. CIRCULAR LINKED LIST: Week days =================

struct DayNode {
    string day;
    DayNode* next;
};

class DayCycle {
private:
    DayNode* last;               // last node. last->next is the FIRST node.
public:
    DayCycle() { last = nullptr; }
    ~DayCycle() {
        if (last == nullptr) return;
        DayNode* cur = last->next;
        last->next = nullptr;                // break the circle, then delete one by one
        while (cur != nullptr) {
            DayNode* n = cur;
            cur = cur->next;
            delete n;
        }
    }
    DayCycle(const DayCycle&) = delete;
    DayCycle& operator=(const DayCycle&) = delete;

    DayNode* first() const { return (last == nullptr) ? nullptr : last->next; }

    // INSERTION at the end of the cycle
    void insert(const string& d) {
        DayNode* n = new DayNode;
        n->day = d;
        if (last == nullptr) {
            n->next = n;                     // one node points to itself
            last = n;
        } else {
            n->next = last->next;            // new node points to the first
            last->next = n;
            last = n;
        }
    }

    // DELETION by day name
    bool removeDay(const string& d) {
        if (last == nullptr) return false;
        DayNode* prev = last;
        DayNode* cur = last->next;
        do {
            if (cur->day == d) {
                if (cur->next == cur) {      // it was the only node
                    last = nullptr;
                } else {
                    prev->next = cur->next;
                    if (cur == last) last = prev;
                }
                delete cur;
                return true;
            }
            prev = cur;
            cur = cur->next;
        } while (prev != last);
        return false;
    }

    // TRAVERSAL: go once around the circle, starting at "start"
    void printCycle(const DayNode* start) const {
        const DayNode* node = start;
        do {
            cout << node->day << " -> ";
            node = node->next;
        } while (node != start);
        cout << "(back to " << start->day << ")" << endl;
    }
};

void dayMenu(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    DayCycle week;
    week.insert("Mon"); week.insert("Tue"); week.insert("Wed");
    week.insert("Thu"); week.insert("Fri"); week.insert("Sat");

    DayNode* current = week.first();
    string cmd;
    while (current != nullptr) {
        cout << endl << "===== " << current->day << " =====" << endl;
        int shown = 0;
        for (int i = 0; i < timetable.size(); i++) {
            if (timetable[i].day == current->day) {
                printClass(timetable[i]);
                shown++;
            }
        }
        if (shown == 0) cout << "No classes on this day." << endl;

        cout << "n=next day (Sat goes back to Mon)  d=remove this day  w=show the cycle  0=exit: ";
        cin >> cmd;
        cin.ignore(1000, '\n');

        if (cmd == "n") {
            current = current->next;                     // never null: it is a circle
        } else if (cmd == "d") {
            DayNode* nxt = current->next;
            bool onlyOne = (nxt == current);
            string name = current->day;
            week.removeDay(name);
            if (onlyOne) {
                cout << "No days left." << endl;
                current = nullptr;
            } else {
                current = nxt;
            }
        } else if (cmd == "w") {
            week.printCycle(current);
        } else if (cmd == "0") {
            break;
        } else {
            cout << "Unknown command." << endl;
        }
    }
}

// ================= BACKTRACKING: Course planner (fast version) =================

// Do two groups of classes (given by their positions in the timetable) clash?
bool rowsClash(const vector<ClassRecord>& t, const vector<int>& a, const vector<int>& b) {
    for (int i = 0; i < a.size(); i++) {
        for (int j = 0; j < b.size(); j++) {
            if (isOverlap(t[a[i]], t[b[j]])) return true;
        }
    }
    return false;
}

const int MAX_PLANS = 10000;   // stop counting after this many plans (keeps it fast)

// Recursion: choose a section for course number "level", then go to the next course.
void planRec(const vector<ClassRecord>& t,
             const vector<string>& courses,
             const vector<vector<string> >& sections,        // sections[c][s] = name of section s of course c
             const vector<vector<vector<int> > >& rows,      // rows[c][s] = positions of its classes in t
             vector<int>& picked,                            // picked[c] = chosen section number of course c
             int level, int& found, int& backtracks) {

    if (found >= MAX_PLANS) return;                          // enough plans: stop early

    // base case: every course has a section = one clash-free plan
    if (level == (int)courses.size()) {
        found++;
        if (found <= 5) {                                    // show only the first 5 plans
            cout << endl << "Plan " << found << ":" << endl;
            for (int c = 0; c < courses.size(); c++) {
                const vector<int>& r = rows[c][picked[c]];
                for (int k = 0; k < r.size(); k++) printClass(t[r[k]]);
            }
        }
        return;
    }

    // try every section of this course
    for (int s = 0; s < sections[level].size(); s++) {
        bool ok = true;
        for (int j = 0; j < level; j++) {                    // compare with the sections already picked
            if (rowsClash(t, rows[level][s], rows[j][picked[j]])) {
                ok = false;
                break;
            }
        }

        if (ok) {
            picked[level] = s;                                                      // choose
            planRec(t, courses, sections, rows, picked, level + 1, found, backtracks);  // go deeper
        } else {
            backtracks++;                                                           // clash: go back, try next section
        }
    }
}

void plannerMenu(const vector<ClassRecord>& t) {
    cin.ignore(1000, '\n');
    int k;
    cout << "How many courses do you want (1 to 8)? ";
    if (!readInt(k) || k < 1 || k > 8) {
        cout << "Invalid number." << endl;
        return;
    }

    vector<string> courses;                        // the courses the student wants
    vector<vector<string> > sections;              // sections[c][s] = name of a section
    vector<vector<vector<int> > > rows;            // rows[c][s] = positions of that section's classes

    for (int n = 0; n < k; n++) {
        cout << "Course " << n + 1 << " name: ";
        string name;
        getline(cin, name);
        name = toLower(trim(name));

        string real = "";                          // the course name exactly as in the timetable
        for (int i = 0; i < t.size(); i++) {
            if (toLower(t[i].course) == name) { real = t[i].course; break; }
        }
        if (real == "") {
            cout << "Course not found. Planner stopped." << endl;
            return;
        }
        for (int c = 0; c < courses.size(); c++) {
            if (courses[c] == real) {
                cout << "You already added this course. Planner stopped." << endl;
                return;
            }
        }

        vector<string> secs;                       // all different sections of this course
        vector<vector<int> > secRows;              // the classes of each section
        for (int i = 0; i < t.size(); i++) {
            if (t[i].course != real) continue;
            int pos = -1;
            for (int s = 0; s < secs.size(); s++) {
                if (secs[s] == t[i].section) pos = s;
            }
            if (pos == -1) {                       // a new section
                secs.push_back(t[i].section);
                secRows.push_back(vector<int>());
                pos = secs.size() - 1;
            }
            secRows[pos].push_back(i);
        }
        courses.push_back(real);
        sections.push_back(secs);
        rows.push_back(secRows);
    }

    vector<int> picked(courses.size(), 0);
    int found = 0, backtracks = 0;
    planRec(t, courses, sections, rows, picked, 0, found, backtracks);

    cout << endl;
    if (found == 0) cout << "No clash-free plan exists for these courses." << endl;
    else if (found >= MAX_PLANS) cout << MAX_PLANS << "+ clash-free plans exist (first 5 shown)." << endl;
    else cout << found << " clash-free plan(s) found (first 5 shown)." << endl;
    cout << "Backtracks (clashes avoided): " << backtracks << endl;
}

// ================= VIEW ONE SECTION, TEACHER OR ROOM =================

void viewMenu(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    cout << "View the timetable of: 1.Section  2.Teacher  3.Room" << endl;
    cout << "Enter choice: ";
    int pick;
    if (!readInt(pick) || pick < 1 || pick > 3) {
        cout << "Invalid choice." << endl;
        return;
    }
    int field = (pick == 3) ? 4 : pick;          // getField: 1 section, 2 teacher, 4 room

    string name;
    cout << "Name (part of it is enough): ";
    getline(cin, name);
    name = toLower(trim(name));
    if (name == "") {
        cout << "Please type a name." << endl;
        return;
    }

    vector<ClassRecord> found;                   // linear search
    for (int i = 0; i < timetable.size(); i++) {
        if (toLower(getField(timetable[i], field)).find(name) != string::npos) {
            found.push_back(timetable[i]);
        }
    }
    if (found.empty()) {
        cout << "No classes found for " << name << "." << endl;
        return;
    }

    mergeSortBy(found, 1);                       // order by day, then start time

    string lastDay = "";
    for (int i = 0; i < found.size(); i++) {
        if (found[i].day != lastDay) {
            cout << endl << "=== " << found[i].day << " ===" << endl;
            lastDay = found[i].day;
        }
        cout << "  " << minutesToTime(found[i].startMin) << "-" << minutesToTime(found[i].endMin)
             << "  " << found[i].course << " | " << found[i].teacher << " | "
             << found[i].section << " | " << found[i].room;

        bool clash = false;                      // does it overlap an earlier class?
        for (int j = 0; j < i; j++) {
            if (isOverlap(found[i], found[j])) clash = true;
        }
        if (clash) cout << "   <-- CLASH";
        cout << endl;
    }
    cout << endl << found.size() << " class(es) found." << endl;
}

// ================= BINARY SEARCH TREE: Course index =================

struct BstNode {
    string name;           // course name
    int classCount;        // how many classes this course has
    BstNode* left;         // smaller names
    BstNode* right;        // bigger names
};

class CourseIndex {
private:
    BstNode* root;
    int count;

    void destroy(BstNode* n) {                    // delete every node (recursion)
        if (n == nullptr) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    BstNode* insertRec(BstNode* n, const string& name) {
        if (n == nullptr) {                       // empty place: make the node here
            BstNode* x = new BstNode;
            x->name = name;
            x->classCount = 1;
            x->left = nullptr;
            x->right = nullptr;
            count++;
            return x;
        }
        string a = toLower(name), b = toLower(n->name);
        if (a == b) n->classCount++;              // same course again
        else if (a < b) n->left = insertRec(n->left, name);
        else n->right = insertRec(n->right, name);
        return n;
    }

    void inorderRec(const BstNode* n, int& number) const {   // left, node, right = A to Z
        if (n == nullptr) return;
        inorderRec(n->left, number);
        cout << number++ << ". " << n->name << " (" << n->classCount << " classes)" << endl;
        inorderRec(n->right, number);
    }

    int heightRec(const BstNode* n) const {
        if (n == nullptr) return 0;
        return 1 + max(heightRec(n->left), heightRec(n->right));
    }

    BstNode* removeRec(BstNode* n, const string& key, bool& removed) {
        if (n == nullptr) return nullptr;
        string b = toLower(n->name);
        if (key < b) {
            n->left = removeRec(n->left, key, removed);
        } else if (key > b) {
            n->right = removeRec(n->right, key, removed);
        } else {
            removed = true;
            if (n->left == nullptr) {             // 0 or 1 child
                BstNode* r = n->right;
                delete n;
                return r;
            }
            if (n->right == nullptr) {
                BstNode* l = n->left;
                delete n;
                return l;
            }
            BstNode* m = n->right;                // 2 children: take the smallest on the right
            while (m->left != nullptr) m = m->left;
            n->name = m->name;
            n->classCount = m->classCount;
            bool dummy = false;
            n->right = removeRec(n->right, toLower(m->name), dummy);
        }
        return n;
    }

public:
    CourseIndex() { root = nullptr; count = 0; }
    ~CourseIndex() { destroy(root); }
    CourseIndex(const CourseIndex&) = delete;
    CourseIndex& operator=(const CourseIndex&) = delete;

    int size() const { return count; }
    int height() const { return heightRec(root); }

    void insert(const string& name) { root = insertRec(root, name); }

    // SEARCH: go left or right. "steps" counts the checks.
    const BstNode* search(const string& name, int& steps) const {
        string key = toLower(name);
        const BstNode* cur = root;
        steps = 0;
        while (cur != nullptr) {
            steps++;
            string b = toLower(cur->name);
            if (key == b) return cur;
            cur = (key < b) ? cur->left : cur->right;
        }
        return nullptr;
    }

    void printInOrder() const {
        int number = 1;
        inorderRec(root, number);
    }

    bool remove(const string& name) {
        bool removed = false;
        root = removeRec(root, toLower(name), removed);
        if (removed) count--;
        return removed;
    }
};

void courseIndexMenu(const vector<ClassRecord>& timetable) {
    cin.ignore(1000, '\n');
    CourseIndex index;                            // built fresh from the timetable
    for (int i = 0; i < timetable.size(); i++) index.insert(timetable[i].course);

    int ch = -1;
    while (ch != 0) {
        cout << endl << "--- Course index (BST): " << index.size() << " courses ---" << endl;
        cout << "1. Show all courses A to Z (in-order)" << endl;
        cout << "2. Search a course" << endl;
        cout << "3. Remove a course from the index" << endl;
        cout << "4. Tree height" << endl;
        cout << "0. Back" << endl;
        cout << "Enter choice: ";
        if (!readInt(ch)) { ch = -1; cout << "Please enter a number." << endl; continue; }

        if (ch == 1) {
            index.printInOrder();
        } else if (ch == 2) {
            string name;
            cout << "Exact course name: ";
            getline(cin, name);
            name = trim(name);
            int steps = 0;
            const BstNode* found = index.search(name, steps);
            if (found == nullptr) cout << "Not found. (" << steps << " checks)" << endl;
            else cout << "Found: " << found->name << " with " << found->classCount
                      << " classes. (" << steps << " checks)" << endl;
        } else if (ch == 3) {
            string name;
            cout << "Exact course name to remove: ";
            getline(cin, name);
            if (index.remove(trim(name))) cout << "Removed from the index (the timetable is not changed)." << endl;
            else cout << "Not found." << endl;
        } else if (ch == 4) {
            cout << "Height: " << index.height() << " levels for " << index.size() << " courses." << endl;
        }
    }
}

// ================= GRAPH: Clash graph =================
struct EdgeNode {
    int to;                // the other class (its position in the vector)
    EdgeNode* next;
};

class ClashGraph {
private:
    int n;                 // number of dots
    int edges;             // number of lines
    EdgeNode** adj;        // adj[i] = list of classes that clash with class i
public:
    ClashGraph(int vertices) {
        n = vertices;
        edges = 0;
        adj = new EdgeNode*[n];
        for (int i = 0; i < n; i++) adj[i] = nullptr;
    }
    ~ClashGraph() {
        for (int i = 0; i < n; i++) {
            while (adj[i] != nullptr) {
                EdgeNode* e = adj[i];
                adj[i] = e->next;
                delete e;
            }
        }
        delete[] adj;
    }
    ClashGraph(const ClashGraph&) = delete;
    ClashGraph& operator=(const ClashGraph&) = delete;

    int edgeCount() const { return edges; }

    void addEdge(int a, int b) {                  // a line works both ways
        EdgeNode* e1 = new EdgeNode;
        e1->to = b;
        e1->next = adj[a];
        adj[a] = e1;
        EdgeNode* e2 = new EdgeNode;
        e2->to = a;
        e2->next = adj[b];
        adj[b] = e2;
        edges++;
    }

    const EdgeNode* firstEdge(int v) const { return adj[v]; }

    int degree(int v) const {                     // how many classes clash with v
        int d = 0;
        for (const EdgeNode* e = adj[v]; e != nullptr; e = e->next) d++;
        return d;
    }
};

void graphMenu(const vector<ClassRecord>& t) {
    int n = t.size();
    if (n == 0) {
        cout << "Timetable is empty." << endl;
        return;
    }

    ClashGraph g(n);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (!isOverlap(t[i], t[j]) || isSharedClass(t[i], t[j])) continue;
            if (t[i].room == t[j].room || t[i].teacher == t[j].teacher ||
                t[i].section == t[j].section) {
                g.addEdge(i, j);
            }
        }
    }

    cout << "Clash graph: " << n << " classes (dots), " << g.edgeCount()
         << " clashes (lines)." << endl;
    if (g.edgeCount() == 0) {
        cout << "No clashes. The graph has no lines." << endl;
        return;
    }

    bool* seen = new bool[n];
    int* queue = new int[n];
    for (int i = 0; i < n; i++) seen[i] = false;

    int groups = 0;
    for (int start = 0; start < n; start++) {
        if (seen[start] || g.degree(start) == 0) continue;

        groups++;
        cout << endl << "Clash group " << groups << ":" << endl;
        int head = 0, tail = 0;
        queue[tail++] = start;
        seen[start] = true;

        while (head < tail) {
            int v = queue[head++];
            cout << "  ";
            printClass(t[v]);
            cout << "     clashes with " << g.degree(v) << " class(es)" << endl;
            for (const EdgeNode* e = g.firstEdge(v); e != nullptr; e = e->next) {
                if (!seen[e->to]) {
                    seen[e->to] = true;
                    queue[tail++] = e->to;
                }
            }
        }
    }
    cout << endl << groups << " clash group(s) found." << endl;

    delete[] seen;
    delete[] queue;
}

// ================= MAIN: the menu =================

int main() {
    vector<ClassRecord> timetable;

    if (!loadFromCSV("timetable.csv", timetable)) {
        cout << "Error: could not open timetable.csv." << endl;
        cout << "Put it in the same folder as the program." << endl;
        return 1;
    }

    ActionStack undoStack;       // remembers our changes
    RequestQueue requests;       // waiting room-change requests
    MyTimetable myTimetable;     // the student's own classes (singly linked list)

    // Load my saved timetable (if the file exists)
    vector<ClassRecord> saved;
    if (loadFromCSV("my_timetable.csv", saved)) {
        for (int i = 0; i < saved.size(); i++) myTimetable.addEnd(saved[i]);
    }

    int choice = -1;

    while (choice != 0) {
        cout << endl << "===== CampusFlow =====" << endl;
        cout << "1. Show all classes" << endl;
        cout << "2. Search (linear search)" << endl;
        cout << "3. Sort (choose algorithm)" << endl;
        cout << "4. Add a class" << endl;
        cout << "5. Save to file" << endl;
        cout << "6. Remove a class" << endl;
        cout << "7. Clash report" << endl;
        cout << "8. Free classroom finder" << endl;
        cout << "9. Find a course (binary search)" << endl;
        cout << "10. Undo last change (" << undoStack.size() << " available)" << endl;
        cout << "11. Change requests (queue)" << endl;
        cout << "12. My timetable (singly linked list)" << endl;
        cout << "13. Browse classes (doubly linked list)" << endl;
        cout << "14. Week days (circular linked list)" << endl;
        cout << "15. Course planner (backtracking)" << endl;
        cout << "16. View a section, teacher or room" << endl;
        cout << "17. Course index (binary search tree)" << endl;
        cout << "18. Clash graph (BFS)" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            choice = -1;
            cout << "Please enter a number." << endl;
            continue;
        }

        if (choice == 1) {
            printAll(timetable);
        } else if (choice == 2) {
            searchMenu(timetable);
        } else if (choice == 3) {
            sortMenu(timetable);
            undoStack.clear();   // sorting moves classes, so old positions are no longer valid
        } else if (choice == 4) {
            int before = timetable.size();
            addClass(timetable);
            if (timetable.size() == before + 1) {      // a class was really added
                Action a;
                a.type = 1;
                a.index = timetable.size() - 1;
                a.record = timetable.back();
                undoStack.push(a);
            }
        } else if (choice == 5) {
            if (saveToCSV("timetable.csv", timetable)) {
                cout << "Saved successfully." << endl;
            } else {
                cout << "Error: could not save the file." << endl;
            }
        } else if (choice == 6) {
            vector<ClassRecord> copyBefore = timetable;   // keep the old list
            removeClass(timetable);
            recordRemoval(copyBefore, timetable, undoStack);
        } else if (choice == 7) {
            clashReport(timetable);
        } else if (choice == 8) {
            freeRooms(timetable);
        } else if (choice == 9) {
            binarySearchMenu(timetable);
        } else if (choice == 10) {
            undoLast(timetable, undoStack);
        } else if (choice == 11) {
            requestMenu(timetable, requests, undoStack);
        } else if (choice == 12) {
            myTimetableMenu(timetable, myTimetable);
        } else if (choice == 13) {
            browseMenu(timetable);
        } else if (choice == 14) {
            dayMenu(timetable);
        } else if (choice == 15) {
            plannerMenu(timetable);
        } else if (choice == 16) {
            viewMenu(timetable);
        } else if (choice == 17) {
            courseIndexMenu(timetable);
        } else if (choice == 18) {
            graphMenu(timetable);
        } else if (choice == 0) {
            cout << "Goodbye!" << endl;
        } else {
            cout << "Invalid choice." << endl;
        }
    }

    return 0;
}