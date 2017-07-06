/*************************************************************************/
/*  container.cpp                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2017 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2017 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "container.h"
#include "message_queue.h"
#include "scene/scene_string_names.h"

void Container::_child_minsize_changed() {

	Size2 ms = get_combined_minimum_size();
	if (ms.width > get_size().width || ms.height > get_size().height)
		minimum_size_changed();
	queue_sort();
}

void Container::add_child_notify(Node *p_child) {

	Control::add_child_notify(p_child);

	Control *control = p_child->cast_to<Control>();
	if (!control)
		return;

	control->connect("size_flags_changed", this, "queue_sort");
	control->connect("minimum_size_changed", this, "_child_minsize_changed");
	control->connect("visibility_changed", this, "_child_minsize_changed");
	queue_sort();
}

void Container::move_child_notify(Node *p_child) {

	Control::move_child_notify(p_child);

	if (!p_child->cast_to<Control>())
		return;

	queue_sort();
}

void Container::remove_child_notify(Node *p_child) {

	Control::remove_child_notify(p_child);

	Control *control = p_child->cast_to<Control>();
	if (!control)
		return;

	control->disconnect("size_flags_changed", this, "queue_sort");
	control->disconnect("minimum_size_changed", this, "_child_minsize_changed");
	control->disconnect("visibility_changed", this, "_child_minsize_changed");
	queue_sort();
}

void Container::_sort_children() {

	if (!is_inside_tree())
		return;

	notification(NOTIFICATION_SORT_CHILDREN);
	emit_signal(SceneStringNames::get_singleton()->sort_children);
	pending_sort = false;
}

void Container::fit_child_in_rect(Control *p_child, const Rect2 &p_rect) {

	ERR_FAIL_COND(p_child->get_parent() != this);

	Size2 minsize = p_child->get_combined_minimum_size();
	Rect2 r = p_rect;

	if (!(p_child->get_h_size_flags() & SIZE_FILL)) {
		r.size.x = minsize.x;
		r.position.x += Math::floor((p_rect.size.x - minsize.x) / 2);
	}

	if (!(p_child->get_v_size_flags() & SIZE_FILL)) {
		r.size.y = minsize.y;
		r.position.y += Math::floor((p_rect.size.y - minsize.y) / 2);
	}

	for (int i = 0; i < 4; i++)
		p_child->set_anchor(Margin(i), ANCHOR_BEGIN);

	p_child->set_position(r.position);
	p_child->set_size(r.size);
	p_child->set_rotation(0);
	p_child->set_scale(Vector2(1, 1));
}

void Container::queue_sort() {

	if (!is_inside_tree())
		return;

	if (pending_sort)
		return;

	MessageQueue::get_singleton()->push_call(this, "_sort_children");
	pending_sort = true;
}

void Container::_notification(int p_what) {

	switch (p_what) {

		case NOTIFICATION_ENTER_TREE: {
			pending_sort = false;
			queue_sort();
		} break;
		case NOTIFICATION_RESIZED: {

			queue_sort();
		} break;
		case NOTIFICATION_THEME_CHANGED: {

			queue_sort();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {

			if (is_visible_in_tree()) {
				queue_sort();
			}
		} break;
	}
}

void Container::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_sort_children"), &Container::_sort_children);
	ClassDB::bind_method(D_METHOD("_child_minsize_changed"), &Container::_child_minsize_changed);

	ClassDB::bind_method(D_METHOD("queue_sort"), &Container::queue_sort);
	ClassDB::bind_method(D_METHOD("fit_child_in_rect", "child:Control", "rect"), &Container::fit_child_in_rect);

	BIND_CONSTANT(NOTIFICATION_SORT_CHILDREN);
	ADD_SIGNAL(MethodInfo("sort_children"));
}

Container::Container() {

	pending_sort = false;
}