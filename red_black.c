#include <stdio.h>
#include <stdbool.h>
#define MAX_NAME 30

typedef struct rb_node{
    rb_node* parent; 
    rb_node* right_child; 
    rb_node* left_child; 
    bool is_black; 
    char val[MAX_NAME]; 

}rb_node_t; 

void left_rotate(rb_node_t* node){
    rb_node_t* new_node = node->right_child; 
    node->right_child = new_node->left_child; 
    if(new_node->left_child != NULL){
        new_node->left_child->parent = node; 
    } 
    new_node->parent = node->parent; 
    if(node->parent != NULL){
        
    }
}