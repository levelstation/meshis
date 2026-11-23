CREATE TABLE `download_daily` (
	`day` text NOT NULL,
	`file_id` text NOT NULL,
	`requests` integer DEFAULT 0 NOT NULL,
	PRIMARY KEY(`day`, `file_id`)
);
--> statement-breakpoint
CREATE TABLE `newsletter_send_budget` (
	`minute` text PRIMARY KEY NOT NULL,
	`requests` integer DEFAULT 0 NOT NULL
);
--> statement-breakpoint
CREATE TABLE `newsletter_subscribers` (
	`email` text PRIMARY KEY NOT NULL,
	`status` text NOT NULL,
	`consent_version` text NOT NULL,
	`consented_at` text NOT NULL,
	`confirmed_at` text,
	`confirmation_hash` text,
	`confirmation_expires_at` text,
	`unsubscribe_hash` text NOT NULL,
	`mail_sent_at` text,
	`updated_at` text NOT NULL
);
