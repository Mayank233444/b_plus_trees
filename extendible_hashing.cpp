#include <bits/stdc++.h>
using namespace std;
#define bucket_size 2

struct Bucket
{
    int availabe_element; // bucket to store elements
    int local_depth;
    set<int> bucket_Set;

    Bucket()
    {
        availabe_element = 0;
        local_depth = 0;
    }
};

// find last K bits
int last_k_bits(int num, int k)
{
    if (num == 0)
        return 0;

    int temp = 1; // temp will be of type 0000000000000......0001   32 size
    temp = temp << k;
    temp = temp - 1; // now temp will be of type  0000000000......11111111  , 1 will k times

    return num & temp;
}

//  search fuction
map<int, Bucket *> mp;

Bucket *search(int ele)
{
    if (mp.find(ele) == mp.end())
        return NULL; // if element present it will present
    return mp[ele];
}

map<int, Bucket *> Directory;

int global_depth = 0;

// expand Directory
void expandDirectory()
{
    if (global_depth == 1)
    {
        Directory[1] = Directory[0];
        return;
    }
    else
    {
        for (int i = pow(2, global_depth - 1); i < pow(2, global_depth); i++)
        {
            int val = pow(2, global_depth - 1);
            Directory[i] = Directory[i % val];
        }
    }
}

void merge_buckets(Bucket *b1);

bool check_for_shrink()
{
    for (auto a : Directory)
    {
        if (a.second->local_depth >= global_depth)
            return false;
    }
    return true;
}

// shrink Directory
void shrink()
{
    // condition to shrink
    // all the local depth must be less than global depth
    if (check_for_shrink())
    {
        // removing all the half blocks from directory
        for (int i = pow(2, global_depth - 1); i < pow(2, global_depth); i++)
            Directory.erase(Directory.find(i));
        global_depth--; // now our directory get reduced

        for (auto a : Directory)
            merge_buckets(a.second);
    }
}

// insert function
void insert(int ele)
{
    //     // this will return the pointer to bucket where the element is inserted
    if (search(ele) != NULL)
        return; // that element is already present in the buckets;

    int last_bits_of_ele = last_k_bits(ele, global_depth);
    Bucket *bucket_address;

    for (int i = 0; i < pow(2, global_depth); i++)
    {
        if (last_k_bits(i, global_depth) == last_bits_of_ele)
        {
            bucket_address = Directory[i];
            // cout << bucket_address << "\n";
            break;
        }
    }

    //     // two cases
    //     // first will be one in which bucket have some space left
    if ((bucket_address->availabe_element) != bucket_size)
    {
        bucket_address->availabe_element = bucket_address->availabe_element + 1;
        bucket_address->bucket_Set.insert(ele);
        mp[ele] = bucket_address;
        return;
    }
    //     // other is in which bucket don't have space
    else
    {
        // two cases again
        // local_depth != global_depth
        if (bucket_address->local_depth != global_depth)
        {
            // split the bucket
            // cout << "B\n";
            set<int> st_ele = bucket_address->bucket_Set; // copy of all element of overflow bucket

            Bucket *bucket_to_be_added = new Bucket;

            bucket_address->local_depth = bucket_address->local_depth + 1; // increasing the local depth
            bucket_address->bucket_Set.clear();                            // clearing all element of set;
            bucket_address->availabe_element = 0;                          // making count as zero

            bucket_to_be_added->local_depth = bucket_address->local_depth; // making the new bucket depth same as splitted bucket

            vector<int> vector_of_linked_directoryA_block;

            for (auto a : Directory)
            {
                if (a.second == bucket_address)
                {
                    vector_of_linked_directoryA_block.push_back(a.first);
                }
            }

            int sz = vector_of_linked_directoryA_block.size();

            for (int i = 0; i < vector_of_linked_directoryA_block.size(); i += 2)
            {
                Directory[vector_of_linked_directoryA_block[i]] = bucket_address;
                Directory[vector_of_linked_directoryA_block[i + 1]] = bucket_to_be_added;
            }

            for (auto curr_ele : st_ele)
            {
                Directory[last_k_bits(curr_ele, global_depth)]->bucket_Set.insert(curr_ele);
                Directory[last_k_bits(curr_ele, global_depth)]->availabe_element++;
                mp[curr_ele] = Directory[last_k_bits(curr_ele, global_depth)];
            }
        }
        // local_depth == global_depth
        else
        {
            // extend the directory
            global_depth++;
            expandDirectory();
        }
        // checking again whether we have extra space or not
        insert(ele);
    }
}

// delete function
void delete_ele(int ele)
{
    if (mp.find(ele) == mp.end())
        return; // element does not exist in the disk

    Bucket *bucket_for_deletion_of_element = mp[ele];
    bucket_for_deletion_of_element->availabe_element--;
    bucket_for_deletion_of_element->bucket_Set.erase(bucket_for_deletion_of_element->bucket_Set.find(ele));
    mp.erase(mp.find(ele));

    // try to merge
    merge_buckets(bucket_for_deletion_of_element);
}

void merge_buckets(Bucket *bucket_for_deletion_of_element)
{
    int Directory_row;
    for (auto a : Directory)
    { // searching the above bucket in the Directory
        if (a.second == bucket_for_deletion_of_element)
        {
            Directory_row = a.first;
            break;
        }
    }

    int flag = 1;

    Bucket *other_bucket_to_merge;
    if (Directory_row + pow(2, bucket_for_deletion_of_element->local_depth - 1) >= pow(2, global_depth))
    {
        other_bucket_to_merge = Directory[Directory_row - pow(2, bucket_for_deletion_of_element->local_depth - 1)];
        flag = 0;
    }
    else
    {
        other_bucket_to_merge = Directory[Directory_row + pow(2, bucket_for_deletion_of_element->local_depth - 1)];
    }

    // checking the size of two buckets and their local depth to merge
    if (bucket_for_deletion_of_element->local_depth == other_bucket_to_merge->local_depth && ((bucket_for_deletion_of_element->availabe_element + other_bucket_to_merge->availabe_element) <= bucket_size))
    {
        if (flag == 1)
        {
            for (auto bb : other_bucket_to_merge->bucket_Set)
            {
                bucket_for_deletion_of_element->bucket_Set.insert(bb);
                bucket_for_deletion_of_element->availabe_element++;
                mp[bb] = bucket_for_deletion_of_element;
            }
            bucket_for_deletion_of_element->local_depth--;

            for (auto a : Directory)
            {
                if (a.second == other_bucket_to_merge)
                {
                    Directory[a.first] = bucket_for_deletion_of_element;
                }
            }

            delete (other_bucket_to_merge);
            // other_bucket_to_merge->bucket_Set.clear();
        }
        else
        {
            for (auto bb : bucket_for_deletion_of_element->bucket_Set)
            {
                other_bucket_to_merge->bucket_Set.insert(bb);
                other_bucket_to_merge->availabe_element++;
                mp[bb] = other_bucket_to_merge;
            }
            other_bucket_to_merge->local_depth--;

            for (auto a : Directory)
            {
                if (a.second == bucket_for_deletion_of_element)
                {
                    Directory[a.first] = other_bucket_to_merge;
                }
            }

            delete (bucket_for_deletion_of_element);
        }

        // check for Shrinking of Directory
        shrink();
    }
    // otherwise leave it at that stage only
}

string give_binary(int num)
{
    if (global_depth == 0)
    {
        return "0";
    }

    string ans = "";

    for (int i = 0; i < global_depth; i++)
        ans += '0';

    if (num == 0)
        return ans;

    int c = global_depth - 1;
    while (num > 0)
    {
        if (num % 2)
        {
            ans[c] = '1';
        }
        num = num / 2;
        c--;
    }

    return ans;
}

void print_directory()
{
    cout << "\n********************\n";
    for (auto dd : Directory)
    {
        cout << give_binary(dd.first) << "\t\t";
        for (auto ee : Directory[dd.first]->bucket_Set)
            cout << ee << " ";
        cout << "\n";
    }
    cout << "********************\n";
}

int main()
{
    Directory[0] = new Bucket;

    while (true)
    {
        cout << "Enter your choice : \n";
        cout << "\t1 for Insertion\n";
        cout << "\t2 for Deletion\n";
        cout << "\t3 to print Directory Structure\n";
        cout << "\t4 to Exit\n";
        cout << "\t: ";
        int choice;
        cin >> choice;

        int element_to_perform_operation;
        switch (choice)
        {
        case 1:
            cout << "Enter element to insert : ";
            cin >> element_to_perform_operation;
            insert(element_to_perform_operation);
            break;
        case 2:
            cout << "Enter element to delete : ";
            cin >> element_to_perform_operation;
            delete_ele(element_to_perform_operation);
            break;
        case 3:
            print_directory();
            break;
        case 4:
            exit(0);
        default:
            cout << "Choose from the above given choices only.\n";
        }
        cout << "\n";
    }
    return 0;
}
